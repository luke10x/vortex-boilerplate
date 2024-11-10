Particles
=========

Particles using firangles because
it does not allow to set point size using this ES profile
neither on M1 or Emscripten.

I don't think I will ever use Transform Feedback.
instead I would rely on my shaders to deterministically,
figure out particle states based on time function.