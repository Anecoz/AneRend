// Better, temporally stable box filtering
// [Jimenez14] http://goo.gl/eomGso
// . . . . . . .
// . A . B . C .
// . . D . E . .
// . F . G . H .
// . . I . J . .
// . K . L . M .
// . . . . . . .
vec4 DownsampleBox13Tap(sampler2D tex, vec2 uv, vec2 texelSize)
{
  vec4 A = texture(tex, vec2(uv + texelSize * vec2(-1.0, -1.0)));
  vec4 B = texture(tex, vec2(uv + texelSize * vec2(0.0, -1.0)));
  vec4 C = texture(tex, vec2(uv + texelSize * vec2(1.0, -1.0)));
  vec4 D = texture(tex, vec2(uv + texelSize * vec2(-0.5, -0.5)));
  vec4 E = texture(tex, vec2(uv + texelSize * vec2(0.5, -0.5)));
  vec4 F = texture(tex, vec2(uv + texelSize * vec2(-1.0, 0.0)));
  vec4 G = texture(tex, vec2(uv));
  vec4 H = texture(tex, vec2(uv + texelSize * vec2(1.0, 0.0)));
  vec4 I = texture(tex, vec2(uv + texelSize * vec2(-0.5, 0.5)));
  vec4 J = texture(tex, vec2(uv + texelSize * vec2(0.5, 0.5)));
  vec4 K = texture(tex, vec2(uv + texelSize * vec2(-1.0, 1.0)));
  vec4 L = texture(tex, vec2(uv + texelSize * vec2(0.0, 1.0)));
  vec4 M = texture(tex, vec2(uv + texelSize * vec2(1.0, 1.0)));

  vec2 div = (1.0 / 4.0) * vec2(0.5, 0.125);

  vec4 o = (D + E + I + J) * div.x;
  o += (A + B + G + F) * div.y;
  o += (B + C + H + G) * div.y;
  o += (F + G + L + K) * div.y;
  o += (G + H + M + L) * div.y;

  return o;
}

// 9-tap bilinear upsampler (tent filter)
vec4 UpsampleTent(sampler2D tex, vec2 uv, vec2 texelSize, vec4 sampleScale)
{
  sampleScale = vec4(1.0);
  vec4 d = texelSize.xyxy * vec4(1.0, 1.0, -1.0, 0.0) * sampleScale;

  vec4 s;
  s = texture(tex, uv - d.xy);
  s += texture(tex, uv - d.wy) * 2.0;
  s += texture(tex, uv - d.zy);

  s += texture(tex, uv + d.zw) * 2.0;
  s += texture(tex, uv) * 4.0;
  s += texture(tex, uv + d.xw) * 2.0;

  s += texture(tex, uv + d.zy);
  s += texture(tex, uv + d.wy) * 2.0;
  s += texture(tex, uv + d.xy);

  return s * (1.0 / 16.0);
}