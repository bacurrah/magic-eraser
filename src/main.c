// CSCI411 FINAL PROJECT
// BRADEN CURRAH

#define RAYGUI_IMPLEMENTATION
#include "raylib.h"
#include "raygui.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Stack {
  int count;
  Rectangle* bounds;
  Color** images;
} Stack;

void buildNeighborhood(int* out, int size, int width) {
  int pos = 0;
  int radius = (size - 1) / 2;
  for (int y = -radius; y <= radius; y++) {
    for (int x = -radius; x <= radius; x++) {
      if ((x == 0 && y == 0) || abs(x) + abs(y) > radius) continue;
      out[pos] = (y * width) + x;
      pos++;
    }
  }
}

int getDistance(Color a, Color b) {
  return
    ((a.r - b.r) * (a.r - b.r)) +
    ((a.g - b.g) * (a.g - b.g)) +
    ((a.b - b.b) * (a.b - b.b));
}

int halveIndex(int index, int width) {
  return (index / (2 * width)) * (width / 2) + (index % width) / 2;
}

int findBestMatch(Stack* source, Stack* mask, int level, int maskPos, bool useProxy) {
  int size = source->bounds[level].width * source->bounds[level].height;
  int nSize = 3;

  // Construct neighborhood for current level
  int neighborhoodCurrent[(nSize * nSize) - 1];
  buildNeighborhood(neighborhoodCurrent, nSize, source->bounds[level].width);

  // Construct neighborhood for previous level
  int neighborhoodPrev[(3 * 3) - 1];
  if (level < source->count - 1)
    buildNeighborhood(neighborhoodPrev, 3, source->bounds[level + 1].width);

  int minDistance = INT_MAX;
  int bestMatch = -1;
  // Iterate through every pixel in the source image
  for (int sourcePos = 0; sourcePos < size; sourcePos++) {
    if (mask->images[level][sourcePos].r) continue;
    int sumDistance = 0;
    // Iterate through every neighbor pixel on current level
    for (int i = 0; i < (nSize * nSize) - 1; i++) {
      int indexMask = maskPos + neighborhoodCurrent[i];
      int indexSource = sourcePos + neighborhoodCurrent[i];
      // The index is out of bounds, use a random value
      if (indexMask < 0 || indexMask >= size || indexSource < 0 || indexSource >= size) {
        sumDistance += rand() % (255 * 3);
        continue;
      };
      if (mask->images[level][indexMask].r) continue;
      sumDistance += getDistance(source->images[level][indexMask], source->images[level][indexSource]);
    }

    if (useProxy && level != source->count - 1) {
      int prevSize = source->bounds[level + 1].width * source->bounds[level + 1].height;
      int prevMaskPos = halveIndex(maskPos, source->bounds[level].width);
      int prevSourcePos = halveIndex(sourcePos, source->bounds[level].width);
      // Iterate through every neighbor pixel on previous level
      for (int i = 0; i < (3 * 3) - 1; i++) {
        int indexMask = prevMaskPos + neighborhoodPrev[i];
        int indexSource = prevSourcePos + neighborhoodPrev[i];
        // The index is out of bounds, use a random value
        if (indexMask < 0 || indexMask >= prevSize || indexSource < 0 || indexSource >= prevSize) {
          sumDistance += rand() % (255 * 3);
          continue;
        }
        sumDistance += getDistance(source->images[level + 1][indexMask], source->images[level + 1][indexSource]);
      }
    }

    if (sumDistance < minDistance) {
      minDistance = sumDistance;
      bestMatch = sourcePos;
    }
  }

  return bestMatch;
}

void generateImage(Stack* source, Stack* mask, bool useProxy, bool drawImage, bool drawHeatmap) {
  Image heatMap;

  // Iterate through each pyramid (proxy) level
  for (int level = source->count - 1; level >= 0; level--) {
    if (level != 0 && !useProxy) continue;
    int size = mask->bounds[level].width * mask->bounds[level].height;

    if (drawHeatmap)
      heatMap = GenImageColor(source->bounds[level].width, source->bounds[level].height, BLANK);

    // Iterate through every pixel in the selection
    for (int maskPos = 0; maskPos < size; maskPos++) {
      if (mask->images[level][maskPos].r == 0) continue;
      // Find the best candidate pixel for this index
      int bestMatch = findBestMatch(source, mask, level, maskPos, useProxy);

      source->images[level][maskPos] = source->images[level][bestMatch];
      mask->images[level][maskPos] = (Color){0, 0, 0, 0};

      // Update the image in realtime. This involves per-frame upload to the GPU,
      // so there will be a not insignificant performance impact when this is on.
      if (drawImage) {
        Image screenImage = { .data = source->images[level], .width = source->bounds[level].width, .height = source->bounds[level].height, .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, .mipmaps = 1 };
        Texture2D screenImageTemp = LoadTextureFromImage(screenImage);
        Texture2D heatMapTemp;

        if (drawHeatmap) {
          ImageDrawPixel(&heatMap, bestMatch % (int)source->bounds[level].width, bestMatch / source->bounds[level].width, WHITE);
          heatMapTemp = LoadTextureFromImage(heatMap);
        }

        BeginDrawing();
        DrawTexture(screenImageTemp, source->bounds[level].x, source->bounds[level].y, WHITE);
        if (drawHeatmap) DrawTexture(heatMapTemp, source->bounds[level].x, source->bounds[level].y, WHITE);
        EndDrawing();
        UnloadTexture(screenImageTemp);
        if (drawHeatmap) UnloadTexture(heatMapTemp);
      }
    }
  }
}

#define PADDING 8
#define MENU_WIDTH 152
#define LIST_HEIGHT 184
#define IMAGE_WIDTH 512
#define IMAGE_HEIGHT 512

typedef enum BrushType {
  SMALL  = 0x1,
  MEDIUM = 0x2,
  LARGE  = 0x4,
  SELECT = 0x8
} BrushType;

int main() {
  Rectangle windowBounds = {0, 0, (4 * PADDING) + MENU_WIDTH + (IMAGE_WIDTH * 1.5), (2 * PADDING) + IMAGE_HEIGHT}; // 952, 528
  Rectangle menuBounds   = {PADDING, PADDING, MENU_WIDTH, windowBounds.height - (2 * PADDING)};
  Rectangle listBounds   = {menuBounds.x + PADDING, menuBounds.y + 24 + PADDING, MENU_WIDTH - (2 * PADDING), LIST_HEIGHT};
  Rectangle brushBounds  = {menuBounds.x + PADDING, listBounds.y + listBounds.height + (2 * PADDING), MENU_WIDTH - (2 * PADDING), 24 + (2 * PADDING)};
  Rectangle resetBounds  = {menuBounds.x + PADDING, brushBounds.y + brushBounds.height + PADDING, MENU_WIDTH - (2 * PADDING), 24};
  Rectangle startBounds  = {menuBounds.x + PADDING, resetBounds.y + resetBounds.height + PADDING, MENU_WIDTH - (2 * PADDING), 48};
  Rectangle exitBounds   = {menuBounds.x + PADDING, startBounds.y + startBounds.height + PADDING, MENU_WIDTH - (2 * PADDING), 24};
  Rectangle proxyBounds  = {menuBounds.x + PADDING, exitBounds.y + exitBounds.height + PADDING, 24, 24};
  Rectangle showBounds   = {menuBounds.x + PADDING, proxyBounds.y + proxyBounds.height + PADDING, 24, 24};
  Rectangle heatBounds   = {menuBounds.x + PADDING, showBounds.y + showBounds.height + PADDING, 24, 24};
  Rectangle imageBounds  = {menuBounds.width + (2 * PADDING), PADDING, IMAGE_WIDTH, IMAGE_HEIGHT};
  Rectangle proxy1Bounds = {imageBounds.x + imageBounds.width + PADDING, PADDING, imageBounds.width / 2, imageBounds.height / 2};
  Rectangle proxy2Bounds = {proxy1Bounds.x, proxy1Bounds.y + proxy1Bounds.height + PADDING, imageBounds.width / 4, imageBounds.height / 4};
  Rectangle proxy3Bounds = {proxy1Bounds.x, proxy2Bounds.y + proxy2Bounds.height + PADDING, imageBounds.width / 8, imageBounds.height / 8};

  Color* image  = NULL;
  Color* proxy1 = NULL;
  Color* proxy2 = NULL;
  Color* proxy3 = NULL;
  Texture2D imageTexture;
  Texture2D proxy1Texture;
  Texture2D proxy2Texture;
  Texture2D proxy3Texture;

  bool maskInit = 0;
  Rectangle maskBounds = {0, 0, 0, 0};

  RenderTexture2D mask;
  FilePathList files = LoadDirectoryFilesEx("images", ".png", 1);

  int chosenFile = -1;
  int activeFile = -1;
  int scrollPos  = 0;

  BrushType brushType = MEDIUM;
  bool brush1 = 0;
  bool brush2 = 1;
  bool brush3 = 0;
  bool brush4 = 0;

  bool useProxies = 1;
  bool liveUpdate = 1;
  bool showHeatmap = 0;

  SetTraceLogLevel(LOG_ERROR);
  SetConfigFlags(FLAG_WINDOW_UNDECORATED);
  InitWindow(windowBounds.width, windowBounds.height, "Magic Eraser Demo");

  while (!WindowShouldClose()) {
    if (IsKeyPressed(KEY_ESCAPE)) break;

    BeginDrawing();

    ClearBackground(BLACK);
    GuiPanel(menuBounds, "MAGIC ERASER DEMO");
    GuiListViewEx(listBounds, files.paths, files.count, &scrollPos, &chosenFile, NULL);

    GuiGroupBox(brushBounds, "Brush Size");
    GuiToggle((Rectangle){ brushBounds.x + (1 * PADDING), brushBounds.y + PADDING, 24, 24 }, "#103#", &brush1);
    if (brush1 + (brush2 << 1) + (brush3 << 2) + (brush4 << 3) != brushType) {
      brush1 = 1; brush2 = brush3 = brush4 = 0; brushType = SMALL;
    }
    GuiToggle((Rectangle){ brushBounds.x + (2 * PADDING) + 24, brushBounds.y + PADDING, 24, 24 }, "#104#", &brush2);
    if (brush1 + (brush2 << 1) + (brush3 << 2) + (brush4 << 3) != brushType) {
      brush2 = 1; brush1 = brush3 = brush4 = 0; brushType = MEDIUM;
    }
    GuiToggle((Rectangle){ brushBounds.x + (3 * PADDING) + 48, brushBounds.y + PADDING, 24, 24 }, "#105#", &brush3);
    if (brush1 + (brush2 << 1) + (brush3 << 2) + (brush4 << 3) != brushType) {
      brush3 = 1; brush1 = brush2 = brush4 = 0; brushType = LARGE;
    }
    GuiToggle((Rectangle){ brushBounds.x + (4 * PADDING) + 72, brushBounds.y + PADDING, 24, 24 }, "#106#", &brush4);
    if (brush1 + (brush2 << 1) + (brush3 << 2) + (brush4 << 3) != brushType) {
      brush4 = 1; brush1 = brush2 = brush3 = 0; brushType = SELECT;
    }

    if (GuiButton(resetBounds, "CLEAR SELECTION")) {
      maskInit = 0;
      BeginTextureMode(mask);
      ClearBackground(BLANK);
      EndTextureMode();
    }

    if (GuiButton(startBounds, "GENERATE") && activeFile >= 0) {
      Stack sourceStack = {
        .count = 4,
        .bounds = (Rectangle[]) {imageBounds, proxy1Bounds, proxy2Bounds, proxy3Bounds},
        .images = (Color*[]){image, proxy1, proxy2, proxy3},
      };

      Image maskTemp = LoadImageFromTexture(mask.texture);
      ImageFlipVertical(&maskTemp);
      Image maskProxy1Temp = ImageCopy(maskTemp);
      Image maskProxy2Temp = ImageCopy(maskTemp);
      Image maskProxy3Temp = ImageCopy(maskTemp);

      ImageResize(&maskProxy1Temp, maskTemp.width / 2, maskTemp.height / 2);
      ImageResize(&maskProxy2Temp, maskTemp.width / 4, maskTemp.height / 4);
      ImageResize(&maskProxy3Temp, maskTemp.width / 8, maskTemp.height / 8);

      // Texture2D maskProxy1Texture = LoadTextureFromImage(maskProxy1Temp);
      // Texture2D maskProxy2Texture = LoadTextureFromImage(maskProxy2Temp);
      // Texture2D maskProxy3Texture = LoadTextureFromImage(maskProxy3Temp);

      // DrawTexture(maskProxy1Texture, proxy1Bounds.x, proxy1Bounds.y, WHITE);
      // DrawTexture(maskProxy2Texture, proxy2Bounds.x, proxy2Bounds.y, WHITE);
      // DrawTexture(maskProxy3Texture, proxy3Bounds.x, proxy3Bounds.y, WHITE);
      // EndDrawing();

      Color* maskImage  = LoadImageColors(maskTemp);
      Color* maskProxy1 = LoadImageColors(maskProxy1Temp);
      Color* maskProxy2 = LoadImageColors(maskProxy2Temp);
      Color* maskProxy3 = LoadImageColors(maskProxy3Temp);

      UnloadImage(maskTemp);
      UnloadImage(maskProxy1Temp);
      UnloadImage(maskProxy2Temp);
      UnloadImage(maskProxy3Temp);

      Stack maskStack = {
        .count = 4,
        .bounds = sourceStack.bounds,
        .images = (Color*[]){maskImage, maskProxy1, maskProxy2, maskProxy3}
      };

      EndDrawing();
      generateImage(&sourceStack, &maskStack, useProxies, liveUpdate, showHeatmap);

      BeginTextureMode(mask);
      ClearBackground(BLANK);
      EndTextureMode();
      maskInit = 0;

      Image finalImage = { .data = image, .width = imageBounds.width, .height = imageBounds.height, .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, .mipmaps = 1 };
      UnloadTexture(imageTexture);
      imageTexture = LoadTextureFromImage(finalImage);
    }

    if (GuiButton(exitBounds, "EXIT")) break;

    if(GuiCheckBox(proxyBounds, "USE PROXIES", &useProxies)) {
      if (useProxies) windowBounds.width = (imageBounds.width * 1.5) + (4 * PADDING) + MENU_WIDTH;
      else windowBounds.width = imageBounds.width + (3 * PADDING) + MENU_WIDTH;
      SetWindowSize(windowBounds.width, windowBounds.height);
      SetWindowPosition(
        (GetMonitorWidth(GetCurrentMonitor()) / 2.) - (windowBounds.width / 2.),
        (GetMonitorHeight(GetCurrentMonitor()) / 2.) - (windowBounds.height / 2.)
      );
    }
    GuiCheckBox(showBounds, "LIVE UPDATE", &liveUpdate);
    if (liveUpdate) GuiCheckBox(heatBounds, "SHOW HEATMAP", &showHeatmap);

    DrawText("BY BRADEN CURRAH 2024", menuBounds.x + PADDING, menuBounds.y + menuBounds.height - PADDING - 8, 8, GRAY);
    DrawText("NO IMAGE SELECTED", imageBounds.x + (imageBounds.width / 2) - (MeasureText("NO IMAGE SELECTED", 8) / 2.), imageBounds.y + (imageBounds.height / 2) - 4, 8, WHITE);
    GuiGroupBox(imageBounds, NULL);
    if (useProxies) {
      GuiGroupBox(proxy1Bounds, NULL);
      GuiGroupBox(proxy2Bounds, NULL);
      GuiGroupBox(proxy3Bounds, NULL);
    }

    if (activeFile >= 0) {
      DrawTexture(imageTexture, imageBounds.x, imageBounds.y, WHITE);
      if (useProxies) {
        DrawTexture(proxy1Texture, proxy1Bounds.x, proxy1Bounds.y, WHITE);
        DrawTexture(proxy2Texture, proxy2Bounds.x, proxy2Bounds.y, WHITE);
        DrawTexture(proxy3Texture, proxy3Bounds.x, proxy3Bounds.y, WHITE);
      }
      DrawTextureRec(mask.texture, (Rectangle){0, 0, imageBounds.width, -imageBounds.height}, (Vector2){imageBounds.x, imageBounds.y,}, (Color){255, 0, 0, 80});
      if (maskInit) GuiGroupBox(maskBounds, TextFormat("%.0fx%.0f", maskBounds.width, maskBounds.height));

      Vector2 cursor = GetMousePosition();
      if (CheckCollisionPointRec(cursor, imageBounds)) {
        const int idx = ((int)(cursor.x - imageBounds.x) + (imageBounds.width * (int)(cursor.y - imageBounds.y)));
        const float brushSize = 4.0 * brushType;
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
          if (maskInit) {
            if (cursor.x - brushSize >= imageBounds.x && cursor.x - brushSize < maskBounds.x) {
              maskBounds.width = maskBounds.width + (maskBounds.x - (cursor.x - brushSize));
              maskBounds.x = cursor.x - brushSize;
            }
            if (cursor.y - brushSize >= imageBounds.y && cursor.y - brushSize < maskBounds.y) {
              maskBounds.height = maskBounds.height + (maskBounds.y - (cursor.y - brushSize));
              maskBounds.y = cursor.y - brushSize;
            }
            if (cursor.x + brushSize <= imageBounds.x + imageBounds.width && cursor.x + brushSize > maskBounds.x + maskBounds.width) {
              maskBounds.width = (cursor.x + brushSize) - maskBounds.x;
            }
            if (cursor.y + brushSize <= imageBounds.y + imageBounds.height && cursor.y + brushSize > maskBounds.y + maskBounds.height) {
              maskBounds.height = (cursor.y + brushSize) - maskBounds.y;
            }
          }
          else {
            maskInit = 1;
            maskBounds.x = cursor.x - brushSize;
            maskBounds.y = cursor.y - brushSize;
            maskBounds.width = 2.0 * brushSize;
            maskBounds.height = 2.0 * brushSize;
          }

          if (maskBounds.x < imageBounds.x) maskBounds.x = imageBounds.x;
          if (maskBounds.y < imageBounds.y) maskBounds.y = imageBounds.y;

          BeginTextureMode(mask);
          DrawCircle(cursor.x - imageBounds.x, cursor.y - imageBounds.y, brushSize, WHITE);
          EndTextureMode();
        }
        DrawText(TextFormat("X: %.0f Y: %.0f", cursor.x - imageBounds.x, cursor.y - imageBounds.y), imageBounds.x + PADDING, imageBounds.y + imageBounds.height - PADDING - 20, 8, WHITE);
        DrawText(TextFormat("R: %d G: %d B: %d", image[idx].r, image[idx].g, image[idx].b), imageBounds.x + PADDING, imageBounds.y + imageBounds.height - PADDING - 8, 8, WHITE);
      }
    }

    EndDrawing();

    if (chosenFile >= 0 && chosenFile != activeFile) {
      activeFile = chosenFile;
      maskInit = 0;

      UnloadRenderTexture(mask);
      UnloadTexture(imageTexture);
      UnloadTexture(proxy1Texture);
      UnloadTexture(proxy2Texture);
      UnloadTexture(proxy3Texture);
      if (image) UnloadImageColors(image);
      if (proxy1) UnloadImageColors(proxy1);
      if (proxy2) UnloadImageColors(proxy2);
      if (proxy3) UnloadImageColors(proxy3);

      Image imageTemp = LoadImage(files.paths[activeFile]);
      image = LoadImageColors(imageTemp);
      imageTexture = LoadTextureFromImage(imageTemp);
      mask = LoadRenderTexture(imageTemp.width, imageTemp.height);

      imageBounds.width = imageTemp.width;
      imageBounds.height = imageTemp.height;
      if (useProxies) windowBounds.width = (imageBounds.width * 1.5) + (4 * PADDING) + MENU_WIDTH;
      else windowBounds.width = imageBounds.width + (3 * PADDING) + MENU_WIDTH;
      windowBounds.height = imageTemp.height + (2 * PADDING);
      menuBounds.height = windowBounds.height - (2 * PADDING);

      proxy1Bounds = (Rectangle){imageBounds.x + imageBounds.width + PADDING, PADDING, imageBounds.width / 2, imageBounds.height / 2};
      proxy2Bounds = (Rectangle){proxy1Bounds.x, proxy1Bounds.y + proxy1Bounds.height + PADDING, imageBounds.width / 4, imageBounds.height / 4};
      proxy3Bounds = (Rectangle){proxy1Bounds.x, proxy2Bounds.y + proxy2Bounds.height + PADDING, imageBounds.width / 8, imageBounds.height / 8};

      Image proxy1Temp = ImageCopy(imageTemp);
      Image proxy2Temp = ImageCopy(imageTemp);
      Image proxy3Temp = ImageCopy(imageTemp);

      ImageResize(&proxy1Temp, imageTemp.width / 2, imageTemp.height / 2);
      ImageResize(&proxy2Temp, imageTemp.width / 4, imageTemp.height / 4);
      ImageResize(&proxy3Temp, imageTemp.width / 8, imageTemp.height / 8);

      proxy1 = LoadImageColors(proxy1Temp);
      proxy2 = LoadImageColors(proxy2Temp);
      proxy3 = LoadImageColors(proxy3Temp);

      proxy1Texture = LoadTextureFromImage(proxy1Temp);
      proxy2Texture = LoadTextureFromImage(proxy2Temp);
      proxy3Texture = LoadTextureFromImage(proxy3Temp);

      UnloadImage(imageTemp);
      UnloadImage(proxy1Temp);
      UnloadImage(proxy2Temp);
      UnloadImage(proxy3Temp);

      SetWindowSize(windowBounds.width, windowBounds.height);
      SetWindowPosition(
        (GetMonitorWidth(GetCurrentMonitor()) / 2.) - (windowBounds.width / 2.),
        (GetMonitorHeight(GetCurrentMonitor()) / 2.) - (windowBounds.height / 2.)
      );
    }
  }

  UnloadRenderTexture(mask);
  UnloadTexture(imageTexture);
  UnloadTexture(proxy1Texture);
  UnloadTexture(proxy2Texture);
  UnloadTexture(proxy3Texture);
  if (image) UnloadImageColors(image);
  if (proxy1) UnloadImageColors(proxy1);
  if (proxy2) UnloadImageColors(proxy1);
  if (proxy3) UnloadImageColors(proxy1);

  CloseWindow();
  return 0;
}
