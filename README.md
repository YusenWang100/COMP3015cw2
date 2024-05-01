# Visual Stuido Version: 
Visual Studio 2022, Windows OS

# Interactivity
Referring to the 3015CW1 operation here, the user can still use the up and down buttons on the keyboard to control the effect of the movement of the puppy in the environment.

# Working Principle
Shadow map

1.Firstly, create a depth map to record the depth information of the scene as seen from the light source.

2.During the rendering stage, render the scene from the light source's point of view into the depth map, recording the depth value of each pixel.

2.Use the near and far planes in the projection matrix to calculate the depth value of each pixel in the depth map from the light source's point of view, obtaining depth information from the light's perspective.

3.Render the scene again in the shadow generation area, but this time from the viewer's perspective.

3.In this rendering pass, by using the depth information calculated from the light source's perspective and the depth value of the current pixel, determine whether the current pixel is in shadow.

3.Finally, based on the shadow test results, adjust the color of the pixel in the shader based on whether it is in shadow, creating the shadow effect.

PBR

1. Use a material system that incorporates physical properties such as albedo (base color), roughness, and metalness to accurately represent how materials react to light in the real world.
2. Utilize image-based lighting (IBL) techniques to capture environmental lighting information using High Dynamic Range (HDR) environment maps.
3. Implement a BRDF (Bidirectional Reflectance Distribution Function) to accurately model how light interacts with surfaces based on their physical properties.
4. Utilize pre-filtered environment maps to create accurate reflections with varying roughness levels.
5. Incorporate IBL irradiance maps to accurately simulate diffuse lighting.
6. Utilize a split-sum approximation technique in the shader to efficiently calculate lighting contributions from multiple light sources.
7. Implement an image-based approach to simulate subsurface scattering for realistic material appearances.
8. Use tone mapping techniques to convert HDR values to values suitable for display on standard monitors.
9. Implement a physically-based sky model to simulate realistic sky and atmospheric lighting effects.
10. Use a post-processing pipeline to apply effects such as gamma correction and FXAA to enhance the final rendering output.



# Youtube video Link
https://youtu.be/ObFLpkL8oXc

# Github Link
https://github.com/YusenWang100/COMP3015cw2.git
