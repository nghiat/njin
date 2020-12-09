struct PSInput {
    float4 p : SV_POSITION;
    float2 uv : TEXCOORD;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

PSInput VSMain(float2 v: V, float2 uv: UV) {
    PSInput input;
    input.p = float4(v, 0.0, 1.0);
    input.uv = uv;
    return input;
}

float4 PSMain(PSInput input) : SV_TARGET {
    return g_texture.Sample(g_sampler, input.uv);
}
