cbuffer cb : register(b0) {
    float4x4 light_mvp;
};

struct PSInput {
    float4 p : SV_POSITION;
};

PSInput VSMain(float4 v: V) {
    PSInput result;
    result.p = mul(v, light_mvp);

    return result;
}
