cbuffer per_obj_cb : register(b0) {
    float4x4 world;
};

cbuffer shared_cb : register(b1) {
    float4x4 light_view;
    float4x4 light_proj;
};

struct PSInput {
    float4 p : SV_POSITION;
};

PSInput VSMain(float4 v: V) {
    PSInput result;
    float4x4 light_mvp = mul(world, mul(light_view, light_proj));
    result.p = mul(v, light_mvp);
    return result;
}
