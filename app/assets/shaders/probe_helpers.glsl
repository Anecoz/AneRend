vec3 getProbeIrradiance(IrradianceProbe probe, vec3 normal)
{
  // Find how much of each direction to retrieve, based on the input normal
  const float EPSILON = 1e-6;
  vec3 outIrr = vec3(0.0);

  // north
  float weight = max(0.0, dot(normal, NORTH_DIR));
  if (weight > EPSILON && probe.irr0.w > EPSILON) {
    outIrr += weight * (probe.irr0.rgb/probe.irr0.w);
  }

  // east
  weight = max(0.0, dot(normal, EAST_DIR));
  if (weight > EPSILON && probe.irr1.w > EPSILON) {
    outIrr += weight * (probe.irr1.rgb/probe.irr1.w);
  }

  // south
  weight = max(0.0, dot(normal, SOUTH_DIR));
  if (weight > EPSILON && probe.irr2.w > EPSILON) {
    outIrr += weight * (probe.irr2.rgb/probe.irr2.w);
  }

  // west
  weight = max(0.0, dot(normal, WEST_DIR));
  if (weight > EPSILON && probe.irr3.w > EPSILON) {
    outIrr += weight * (probe.irr3.rgb/probe.irr3.w);
  }

  // up
  weight = max(0.0, dot(normal, UP_DIR));
  if (weight > EPSILON && probe.irr4.w > EPSILON) {
    outIrr += weight * (probe.irr4.rgb/probe.irr4.w);
  }

  // down
  weight = max(0.0, dot(normal, DOWN_DIR));
  if (weight > EPSILON && probe.irr5.w > EPSILON) {
    outIrr += weight * (probe.irr5.rgb/probe.irr5.w);
  }

  return outIrr;
}