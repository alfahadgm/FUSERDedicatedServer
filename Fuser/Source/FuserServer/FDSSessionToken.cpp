#include "FDSSessionToken.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

// Ed25519 verification: a port of TweetNaCl's crypto_sign_open and its field / group arithmetic (TweetNaCl by
// Bernstein, van Gastel, Janssen, Lange, Schwabe, Smetsers; public domain), with SHA-512 per FIPS 180-4. Constants
// (D, 2D, the base point, sqrt(-1), the group order) were computed exactly for this file.
namespace
{
    typedef int64 gf[16];

    const gf gf0 = {0};
    const gf gf1 = {1};
    const gf D = {0x78a3, 0x1359, 0x4dca, 0x75eb, 0xd8ab, 0x4141, 0x0a4d, 0x0070, 0xe898, 0x7779, 0x4079, 0x8cc7, 0xfe73, 0x2b6f, 0x6cee, 0x5203};
    const gf D2 = {0xf159, 0x26b2, 0x9b94, 0xebd6, 0xb156, 0x8283, 0x149a, 0x00e0, 0xd130, 0xeef3, 0x80f2, 0x198e, 0xfce7, 0x56df, 0xd9dc, 0x2406};
    const gf X = {0xd51a, 0x8f25, 0x2d60, 0xc956, 0xa7b2, 0x9525, 0xc760, 0x692c, 0xdc5c, 0xfdd6, 0xe231, 0xc0a4, 0x53fe, 0xcd6e, 0x36d3, 0x2169};
    const gf Y = {0x6658, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666};
    const gf I = {0xa0b0, 0x4a0e, 0x1b27, 0xc4ee, 0xe478, 0xad2f, 0x1806, 0x2f43, 0xd7a7, 0x3dfb, 0x0099, 0x2b4d, 0xdf0b, 0x4fc1, 0x2480, 0x2b83};
    const int64 L[32] = {0xed, 0xd3, 0xf5, 0x5c, 0x1a, 0x63, 0x12, 0x58, 0xd6, 0x9c, 0xf7, 0xa2, 0xde, 0xf9, 0xde, 0x14,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x10};

#include "FDSSha512Constants.inl"

    // ---------------------------------------------------------------------------------------------------- SHA-512

    uint64 Rotr(uint64 X64, int32 N) { return (X64 >> N) | (X64 << (64 - N)); }

    void Sha512Blocks(uint64 State[8], const uint8* Data, int64 Blocks)
    {
        uint64 W[80];
        for (int64 Block = 0; Block < Blocks; ++Block, Data += 128)
        {
            for (int32 t = 0; t < 16; ++t)
            {
                uint64 Word = 0;
                for (int32 b = 0; b < 8; ++b)
                {
                    Word = (Word << 8) | Data[t * 8 + b];
                }
                W[t] = Word;
            }
            for (int32 t = 16; t < 80; ++t)
            {
                const uint64 S0 = Rotr(W[t - 15], 1) ^ Rotr(W[t - 15], 8) ^ (W[t - 15] >> 7);
                const uint64 S1 = Rotr(W[t - 2], 19) ^ Rotr(W[t - 2], 61) ^ (W[t - 2] >> 6);
                W[t] = W[t - 16] + S0 + W[t - 7] + S1;
            }
            uint64 a = State[0], b = State[1], c = State[2], d = State[3], e = State[4], f = State[5], g = State[6], h = State[7];
            for (int32 t = 0; t < 80; ++t)
            {
                const uint64 T1 = h + (Rotr(e, 14) ^ Rotr(e, 18) ^ Rotr(e, 41)) + ((e & f) ^ (~e & g)) + Sha512K[t] + W[t];
                const uint64 T2 = (Rotr(a, 28) ^ Rotr(a, 34) ^ Rotr(a, 39)) + ((a & b) ^ (a & c) ^ (b & c));
                h = g; g = f; f = e; e = d + T1; d = c; c = b; b = a; a = T1 + T2;
            }
            State[0] += a; State[1] += b; State[2] += c; State[3] += d; State[4] += e; State[5] += f; State[6] += g; State[7] += h;
        }
    }

    void Sha512(uint8 Out[64], const uint8* Data, int64 Length)
    {
        uint64 State[8];
        FMemory::Memcpy(State, Sha512H0, sizeof(State));
        const int64 Full = Length / 128;
        Sha512Blocks(State, Data, Full);
        uint8 Tail[256] = {0};
        const int64 Rest = Length - Full * 128;
        FMemory::Memcpy(Tail, Data + Full * 128, Rest);
        Tail[Rest] = 0x80;
        const int64 TailBlocks = Rest < 112 ? 1 : 2;
        const uint64 Bits = static_cast<uint64>(Length) * 8; // the 128-bit length's high half is 0 here
        for (int32 b = 0; b < 8; ++b)
        {
            Tail[TailBlocks * 128 - 1 - b] = static_cast<uint8>(Bits >> (8 * b));
        }
        Sha512Blocks(State, Tail, TailBlocks);
        for (int32 i = 0; i < 8; ++i)
        {
            for (int32 b = 0; b < 8; ++b)
            {
                Out[i * 8 + b] = static_cast<uint8>(State[i] >> (56 - 8 * b));
            }
        }
    }

    // ---------------------------------------------------------------------------------------------------- field

    void Set(gf r, const gf a) { for (int32 i = 0; i < 16; ++i) r[i] = a[i]; }

    void Carry(gf o)
    {
        for (int32 i = 0; i < 16; ++i)
        {
            o[i] += (1LL << 16);
            const int64 c = o[i] >> 16;
            o[(i + 1) * (i < 15)] += c - 1 + 37 * (c - 1) * (i == 15);
            o[i] -= c * 65536;
        }
    }

    void Select(gf p, gf q, int32 b)
    {
        const int64 c = ~(static_cast<int64>(b) - 1);
        for (int32 i = 0; i < 16; ++i)
        {
            const int64 t = c & (p[i] ^ q[i]);
            p[i] ^= t;
            q[i] ^= t;
        }
    }

    void Pack(uint8* o, const gf n)
    {
        gf m, t;
        Set(t, n);
        Carry(t);
        Carry(t);
        Carry(t);
        for (int32 j = 0; j < 2; ++j)
        {
            m[0] = t[0] - 0xffed;
            for (int32 i = 1; i < 15; ++i)
            {
                m[i] = t[i] - 0xffff - ((m[i - 1] >> 16) & 1);
                m[i - 1] &= 0xffff;
            }
            m[15] = t[15] - 0x7fff - ((m[14] >> 16) & 1);
            const int32 b = static_cast<int32>((m[15] >> 16) & 1);
            m[14] &= 0xffff;
            Select(t, m, 1 - b);
        }
        for (int32 i = 0; i < 16; ++i)
        {
            o[2 * i] = static_cast<uint8>(t[i] & 0xff);
            o[2 * i + 1] = static_cast<uint8>(t[i] >> 8);
        }
    }

    // 0 when equal (constant time), as TweetNaCl's crypto_verify_32.
    int32 Verify32(const uint8* x, const uint8* y)
    {
        uint32 d = 0;
        for (int32 i = 0; i < 32; ++i)
        {
            d |= x[i] ^ y[i];
        }
        return (1 & ((d - 1) >> 8)) - 1;
    }

    int32 NotEqual(const gf a, const gf b)
    {
        uint8 c[32], d[32];
        Pack(c, a);
        Pack(d, b);
        return Verify32(c, d);
    }

    uint8 Parity(const gf a)
    {
        uint8 d[32];
        Pack(d, a);
        return d[0] & 1;
    }

    void Unpack(gf o, const uint8* n)
    {
        for (int32 i = 0; i < 16; ++i)
        {
            o[i] = n[2 * i] + (static_cast<int64>(n[2 * i + 1]) << 8);
        }
        o[15] &= 0x7fff;
    }

    void Add(gf o, const gf a, const gf b) { for (int32 i = 0; i < 16; ++i) o[i] = a[i] + b[i]; }
    void Sub(gf o, const gf a, const gf b) { for (int32 i = 0; i < 16; ++i) o[i] = a[i] - b[i]; }

    void Mul(gf o, const gf a, const gf b)
    {
        int64 t[31] = {0};
        for (int32 i = 0; i < 16; ++i)
        {
            for (int32 j = 0; j < 16; ++j)
            {
                t[i + j] += a[i] * b[j];
            }
        }
        for (int32 i = 0; i < 15; ++i)
        {
            t[i] += 38 * t[i + 16];
        }
        for (int32 i = 0; i < 16; ++i)
        {
            o[i] = t[i];
        }
        Carry(o);
        Carry(o);
    }

    void Square(gf o, const gf a) { Mul(o, a, a); }

    void Pow2523(gf o, const gf i)
    {
        gf c;
        Set(c, i);
        for (int32 a = 250; a >= 0; --a)
        {
            Square(c, c);
            if (a != 1)
            {
                Mul(c, c, i);
            }
        }
        Set(o, c);
    }

    void Invert(gf o, const gf i)
    {
        gf c;
        Set(c, i);
        for (int32 a = 253; a >= 0; --a)
        {
            Square(c, c);
            if (a != 2 && a != 4)
            {
                Mul(c, c, i);
            }
        }
        Set(o, c);
    }

    // ---------------------------------------------------------------------------------------------------- group

    void PointAdd(gf p[4], gf q[4])
    {
        gf a, b, c, d, t, e, f, g, h;
        Sub(a, p[1], p[0]);
        Sub(t, q[1], q[0]);
        Mul(a, a, t);
        Add(b, p[0], p[1]);
        Add(t, q[0], q[1]);
        Mul(b, b, t);
        Mul(c, p[3], q[3]);
        Mul(c, c, D2);
        Mul(d, p[2], q[2]);
        Add(d, d, d);
        Sub(e, b, a);
        Sub(f, d, c);
        Add(g, d, c);
        Add(h, b, a);
        Mul(p[0], e, f);
        Mul(p[1], h, g);
        Mul(p[2], g, f);
        Mul(p[3], e, h);
    }

    void PointSwap(gf p[4], gf q[4], uint8 b)
    {
        for (int32 i = 0; i < 4; ++i)
        {
            Select(p[i], q[i], b);
        }
    }

    void PointPack(uint8* r, gf p[4])
    {
        gf tx, ty, zi;
        Invert(zi, p[2]);
        Mul(tx, p[0], zi);
        Mul(ty, p[1], zi);
        Pack(r, ty);
        r[31] ^= Parity(tx) << 7;
    }

    void ScalarMult(gf p[4], gf q[4], const uint8* s)
    {
        Set(p[0], gf0);
        Set(p[1], gf1);
        Set(p[2], gf1);
        Set(p[3], gf0);
        for (int32 i = 255; i >= 0; --i)
        {
            const uint8 b = (s[i / 8] >> (i & 7)) & 1;
            PointSwap(p, q, b);
            PointAdd(q, p);
            PointAdd(p, p);
            PointSwap(p, q, b);
        }
    }

    void ScalarBase(gf p[4], const uint8* s)
    {
        gf q[4];
        Set(q[0], X);
        Set(q[1], Y);
        Set(q[2], gf1);
        Mul(q[3], X, Y);
        ScalarMult(p, q, s);
    }

    void ModL(uint8* r, int64 x[64])
    {
        int64 carry = 0;
        int32 j = 0;
        for (int32 i = 63; i >= 32; --i)
        {
            carry = 0;
            for (j = i - 32; j < i - 12; ++j)
            {
                x[j] += carry - 16 * x[i] * L[j - (i - 32)];
                carry = (x[j] + 128) >> 8;
                x[j] -= carry * 256;
            }
            x[j] += carry;
            x[i] = 0;
        }
        carry = 0;
        for (j = 0; j < 32; ++j)
        {
            x[j] += carry - (x[31] >> 4) * L[j];
            carry = x[j] >> 8;
            x[j] &= 255;
        }
        for (j = 0; j < 32; ++j)
        {
            x[j] -= carry * L[j];
        }
        for (int32 i = 0; i < 32; ++i)
        {
            x[i + 1] += x[i] >> 8;
            r[i] = static_cast<uint8>(x[i] & 255);
        }
    }

    void Reduce(uint8* r)
    {
        int64 x[64];
        for (int32 i = 0; i < 64; ++i)
        {
            x[i] = static_cast<uint64>(r[i]);
        }
        for (int32 i = 0; i < 64; ++i)
        {
            r[i] = 0;
        }
        ModL(r, x);
    }

    // -A from the encoded public key A (TweetNaCl unpackneg); false if it isn't a curve point.
    bool UnpackNegative(gf r[4], const uint8 p[32])
    {
        gf t, chk, num, den, den2, den4, den6;
        Set(r[2], gf1);
        Unpack(r[1], p);
        Square(num, r[1]);
        Mul(den, num, D);
        Sub(num, num, r[2]);
        Add(den, r[2], den);
        Square(den2, den);
        Square(den4, den2);
        Mul(den6, den4, den2);
        Mul(t, den6, num);
        Mul(t, t, den);
        Pow2523(t, t);
        Mul(t, t, num);
        Mul(t, t, den);
        Mul(t, t, den);
        Mul(r[0], t, den);
        Square(chk, r[0]);
        Mul(chk, chk, den);
        if (NotEqual(chk, num))
        {
            Mul(r[0], r[0], I);
        }
        Square(chk, r[0]);
        Mul(chk, chk, den);
        if (NotEqual(chk, num))
        {
            return false;
        }
        if (Parity(r[0]) == (p[31] >> 7))
        {
            Sub(r[0], gf0, r[0]);
        }
        Mul(r[3], r[0], r[1]);
        return true;
    }

    // ---------------------------------------------------------------------------------------------------- tokens

    TArray<uint8> GPublicKey;
}

bool FDSSessionToken::VerifyEd25519(const uint8* PublicKey, const uint8* Message, int32 MessageLength, const uint8* Signature)
{
    gf p[4], q[4];
    if (MessageLength < 0 || !UnpackNegative(q, PublicKey))
    {
        return false;
    }
    // h = SHA-512(R || A || M) mod L
    TArray<uint8> Buffer;
    Buffer.SetNumUninitialized(64 + MessageLength);
    FMemory::Memcpy(Buffer.GetData(), Signature, 32);
    FMemory::Memcpy(Buffer.GetData() + 32, PublicKey, 32);
    if (MessageLength > 0)
    {
        FMemory::Memcpy(Buffer.GetData() + 64, Message, MessageLength);
    }
    uint8 h[64];
    Sha512(h, Buffer.GetData(), Buffer.Num());
    Reduce(h);
    // S*B - h*A must equal R.
    ScalarMult(p, q, h);
    ScalarBase(q, Signature + 32);
    PointAdd(p, q);
    uint8 t[32];
    PointPack(t, p);
    return Verify32(Signature, t) == 0;
}

bool FDSSessionToken::DecodeBase64Url(const FString& Text, TArray<uint8>& Out)
{
    Out.Reset();
    uint32 Acc = 0;
    int32 Bits = 0;
    for (TCHAR C : Text)
    {
        int32 V;
        if (C >= 'A' && C <= 'Z') V = C - 'A';
        else if (C >= 'a' && C <= 'z') V = C - 'a' + 26;
        else if (C >= '0' && C <= '9') V = C - '0' + 52;
        else if (C == '-') V = 62;
        else if (C == '_') V = 63;
        else return false;
        Acc = (Acc << 6) | static_cast<uint32>(V);
        Bits += 6;
        if (Bits >= 8)
        {
            Bits -= 8;
            Out.Add(static_cast<uint8>((Acc >> Bits) & 0xff));
        }
    }
    return Bits < 6; // a single leftover character can't be valid base64
}

bool FDSSessionToken::SetPublicKey(const FString& Base64Url)
{
    TArray<uint8> Key;
    if (!DecodeBase64Url(Base64Url.TrimStartAndEnd(), Key) || Key.Num() != 32)
    {
        GPublicKey.Reset();
        return false;
    }
    GPublicKey = MoveTemp(Key);
    return true;
}

bool FDSSessionToken::HasPublicKey()
{
    return GPublicKey.Num() == 32;
}

FString FDSSessionToken::Verify(const FString& Token, int64 NowUnixSeconds, FClaims& Out)
{
    if (!HasPublicKey())
    {
        return TEXT("no public key");
    }
    FString Rest = Token;
    if (!Rest.RemoveFromStart(TEXT("psess-")) || !Rest.StartsWith(TEXT("fds1.")) || Rest.Len() > 2048)
    {
        return TEXT("not an FDS session token");
    }
    int32 Dot = INDEX_NONE;
    if (!Rest.FindLastChar(TEXT('.'), Dot) || Dot <= 5)
    {
        return TEXT("malformed token");
    }
    const FString Signed = Rest.Left(Dot);
    TArray<uint8> Signature, Payload;
    if (!DecodeBase64Url(Rest.Mid(Dot + 1), Signature) || Signature.Num() != 64 || !DecodeBase64Url(Signed.Mid(5), Payload))
    {
        return TEXT("malformed token");
    }
    const FTCHARToUTF8 SignedUtf8(*Signed);
    if (!VerifyEd25519(GPublicKey.GetData(), reinterpret_cast<const uint8*>(SignedUtf8.Get()), SignedUtf8.Length(), Signature.GetData()))
    {
        return TEXT("bad signature");
    }
    Payload.Add(0);
    const FString Json = UTF8_TO_TCHAR(reinterpret_cast<const ANSICHAR*>(Payload.GetData()));
    TSharedPtr<FJsonObject> Claims;
    int32 Version = 0;
    double User = 0, Expires = 0;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Claims) || !Claims.IsValid() ||
        !Claims->TryGetNumberField(TEXT("v"), Version) || Version != 1 || !Claims->TryGetStringField(TEXT("slot"), Out.Slot) ||
        !Claims->TryGetStringField(TEXT("gsid"), Out.GameSession) || !Claims->TryGetStringField(TEXT("psid"), Out.PlayerSession) ||
        !Claims->TryGetNumberField(TEXT("uid"), User) || !Claims->TryGetNumberField(TEXT("exp"), Expires))
    {
        return TEXT("malformed claims");
    }
    Out.User = static_cast<int64>(User);
    Out.Expires = static_cast<int64>(Expires);
    if (NowUnixSeconds >= Out.Expires)
    {
        return TEXT("expired");
    }
    return FString();
}
