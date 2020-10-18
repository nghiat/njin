cbuffer cb : register(b0) {
    float4x4 mvp;
    float4 cam;
    float4 obj_color;
    float4 light_pos;
    float4 light_color;
};

struct PSInput {
    float4 p : SV_POSITION;
    float4 v : V;
    float4 n : N;
};

PSInput VSMain(float4 v: V, float4 n: N) {
    PSInput result;
    result.p = mul(v, mvp);
    result.v = v;
    result.n = n;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET {
    float3 light_dir = normalize(light_pos - input.v);
    float3 ambient = 0.1 * light_color;
    float3 diffuse = max(dot(input.n.xyz, light_dir), 0.0) * light_color;
    float3 cam_dir = normalize(cam - input.v);
    float3 reflect_dir = reflect(-light_dir, input.n);
    float spec = pow(max(dot(cam_dir, reflect_dir), 0.0), 32);
    float3 specular = 0.5 * spec * light_color;
    float4 color = float4((ambient + diffuse + specular) * obj_color.xyz, 1.0);
    return color;
}
