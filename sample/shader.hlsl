cbuffer per_obj_cb : register(b0) {
    float4x4 world;
};

cbuffer shared_cb : register(b1) {
    float4x4 view;
    float4x4 proj;
    float4x4 light_view;
    float4x4 light_proj;
    float4 eye_pos;
    float4 obj_color;
    float4 light_pos;
    float4 light_color;
};

struct PSInput {
    float4 p : SV_POSITION;
    float4 v : POSITION0;
    float4 n : NORMAL;
    float4 light_space_p : POSITION1;
};

PSInput VSMain(float4 v: V, float4 n: N) {
    PSInput result;
    float4x4 mvp = mul(world, mul(view, proj));
    result.p = mul(v, mvp);
    result.v = v;
    result.n = n;
    float4x4 light_mvp = mul(world, mul(light_view, light_proj));
    result.light_space_p = mul(v, light_mvp);

    return result;
}

Texture2D g_shadow_texture : register(t0);
SamplerState g_shadow_sampler : register(s0);

float4 PSMain(PSInput input) : SV_TARGET {
    float3 light_dir = normalize(light_pos - input.v);
    float3 ambient = 0.1 * light_color;
    float3 diffuse = max(dot(input.n.xyz, light_dir), 0.0) * light_color;
    float3 eye_dir = normalize(eye_pos - input.v);
    float3 reflect_dir = reflect(-light_dir, input.n);
    float spec = pow(max(dot(eye_dir, reflect_dir), 0.0), 32);
    float3 specular = 0.5 * spec * light_color;
    float4 color = float4((ambient + diffuse + specular) * obj_color.xyz, 1.0);

    // Convert p -> uv
    input.light_space_p = input.light_space_p / input.light_space_p.w;
    float2 shadow_uv;
    shadow_uv.x = (input.light_space_p.x + 1.0) / 2.0;
    shadow_uv.y = 1.0 - (input.light_space_p.y + 1.0) / 2.0;
    if ((saturate(shadow_uv.x) == shadow_uv.x) && (saturate(shadow_uv.y) == shadow_uv.y) && (input.light_space_p.z > 0)) {
      float sample = g_shadow_texture.Sample(g_shadow_sampler, shadow_uv);
      if (input.light_space_p.z > sample + 0.0005)
        return float4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    return color;
}
