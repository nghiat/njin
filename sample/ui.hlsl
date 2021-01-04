cbuffer ui_cb : register(b0) {
    float width;
    float height;
};

struct PSInput {
    float4 p : SV_POSITION;
    float2 uv : TEXCOORD0;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

PSInput VSMain(float2 v: V, float2 uv: UV) {
    PSInput result;
    float x = (v.x - width/2.0) / (width/2.0);
    float y = (v.y - height/2.0) / (height/2.0) * -1.0;
    result.p = float4(x, y, 0.0, 1.0);
    result.uv = uv;
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET {
  float alpha = g_texture.Sample(g_sampler, input.uv);
  return float4(0.0, 0.0, 0.0, alpha);
}
