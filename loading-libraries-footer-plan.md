## Loading Screen Library Footer

1) Collect library names from vendor READMEs (`vendor/afterhours/README.md`, `vendor/pcg_random/README.md`) and craft one concise footer line.
2) Update `src/intro/primary_intro_scene.cpp` (and header if needed) to store the footer string and render it beneath the loading bar using existing font/colors at a small readable size.
3) Verify the footer shows during preload (`Preload`/`PrimaryIntroScene`), keeps the current status/progress layout intact, and tweak spacing if needed.
