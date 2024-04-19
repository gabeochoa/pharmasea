
#include <raylib.h>

#include <iostream>

int main() {
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight,
               "raylib [models] example - models loading");

    // Define the camera to look into our 3d world
    Camera camera = {{0}};
    camera.position = (Vector3){2.0f, 3.0f, 2.0f};  // Camera position
    camera.target = (Vector3){0.0f, 0.6f, 0.0f};    // Camera looking at point
    camera.up = (Vector3){0.0f, 1.0f,
                          0.0f};  // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;          // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;  // Camera mode type

    Model model =
        LoadModel("./resources/models/kennynl/toilet.obj");  // Load model

    Vector3 position = {0.0f, 0.0f, 0.0f};  // Set model position

    BoundingBox bounds =
        GetMeshBoundingBox(model.meshes[0]);  // Set model bounds

    // NOTE: bounds are calculated from the original size of the model,
    // if model is scaled on drawing, bounds must be also scaled

    bool selected = false;  // Selected object flag

    // DisableCursor();  // Limit cursor to relative movement inside the window

    SetTargetFPS(60);  // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())  // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        UpdateCamera(&camera, CAMERA_ORBITAL);

        //----------------------------------------------------------------------------------

        if (IsFileDropped()) {
            FilePathList droppedFiles = LoadDroppedFiles();

            if (droppedFiles.count == 1)  // Only support one file dropped
            {
                if (IsFileExtension(droppedFiles.paths[0], ".obj") ||
                    IsFileExtension(droppedFiles.paths[0], ".gltf") ||
                    IsFileExtension(droppedFiles.paths[0], ".glb") ||
                    IsFileExtension(droppedFiles.paths[0], ".vox") ||
                    IsFileExtension(droppedFiles.paths[0], ".iqm") ||
                    IsFileExtension(droppedFiles.paths[0],
                                    ".m3d"))  // Model file formats supported
                {
                    UnloadModel(model);  // Unload previous model
                    model = LoadModel(droppedFiles.paths[0]);  // Load new model
                    bounds = GetMeshBoundingBox(model.meshes[0]);

                    std::cout << "Loading new file" << std::endl;

                    // TODO: Move camera position from target enough distance to
                    // visualize model properly
                }
            }

            UnloadDroppedFiles(droppedFiles);  // Unload filepaths from memory
        }

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

        ClearBackground(RAYWHITE);

        BeginMode3D(camera);

        DrawModel(model, position, 1.0f, WHITE);  // Draw 3d model with texture

        DrawGrid(20, 10.0f);  // Draw a grid

        if (selected) DrawBoundingBox(bounds, GREEN);  // Draw selection box

        EndMode3D();

        DrawText("Drag & drop model to load mesh/texture.", 10,
                 GetScreenHeight() - 20, 10, DARKGRAY);
        if (selected)
            DrawText("MODEL SELECTED", GetScreenWidth() - 110, 10, 10, GREEN);

        DrawText("(c) Castle 3D model by Alberto Cano", screenWidth - 200,
                 screenHeight - 20, 10, GRAY);

        DrawFPS(10, 10);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadModel(model);  // Unload model

    CloseWindow();  // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
