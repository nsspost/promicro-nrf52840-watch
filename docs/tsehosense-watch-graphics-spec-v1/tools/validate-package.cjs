#!/usr/bin/env node
"use strict";

const fs = require("fs");
const path = require("path");

const root = path.resolve(process.argv[2] || path.join(__dirname, ".."));
const errors = [];

function readJson(relative) {
  try {
    return JSON.parse(fs.readFileSync(path.join(root, relative), "utf8"));
  } catch (error) {
    errors.push(`${relative}: ${error.message}`);
    return null;
  }
}

const required = [
  "README.md",
  "01_VISUAL_LANGUAGE.md",
  "02_TOKENS_AND_TYPOGRAPHY.md",
  "03_LAYOUT_GEOMETRY_256.md",
  "04_COMPONENT_LIBRARY.md",
  "05_SCREEN_CATALOG.md",
  "06_MOTION_PARTIAL_REDRAW.md",
  "07_SKINS_ASSETS.md",
  "08_EMBEDDED_GRAPHICS_C.md",
  "09_CODEX_GRAPHICS_IMPLEMENTATION_PLAN.md",
  "AGENTS_GRAPHICS.md",
  "VALIDATION.md",
  "PACKAGE_MANIFEST.json",
  "tokens/strict-context.tokens.json",
  "layouts/watch-square-256.layout.json",
  "layouts/watch-round-256.layout.json",
  "schemas/graphics-skin.schema.json",
  "reference/atlases/strict-context-watchface-v0.1.1-preview.png",
  "reference/atlases/strict-context-master-atlas-v1.png"
];

for (const relative of required) {
  if (!fs.existsSync(path.join(root, relative))) errors.push(`missing: ${relative}`);
}

const manifest = readJson("PACKAGE_MANIFEST.json");
const tokens = readJson("tokens/strict-context.tokens.json");
const square = readJson("layouts/watch-square-256.layout.json");
const round = readJson("layouts/watch-round-256.layout.json");
readJson("schemas/graphics-skin.schema.json");
readJson("icons/phosphor/ICON_INDEX.json");

function inspectBounds(value, trail = "") {
  if (!value || typeof value !== "object") return;
  if (Array.isArray(value)) {
    value.forEach((item, index) => inspectBounds(item, `${trail}[${index}]`));
    return;
  }
  for (const [key, child] of Object.entries(value)) {
    const here = trail ? `${trail}.${key}` : key;
    if (key.toLowerCase().includes("bounds") && Array.isArray(child) && child.length === 4) {
      const [x, y, w, h] = child;
      if (![x, y, w, h].every(Number.isInteger)) errors.push(`${here}: non-integer bounds`);
      if (w < 0 || h < 0 || x < 0 || y < 0 || x + w > 256 || y + h > 256) {
        errors.push(`${here}: outside 256x256 (${child.join(",")})`);
      }
    }
    inspectBounds(child, here);
  }
}

inspectBounds(square, "square");
inspectBounds(round, "round");

if (tokens?.font?.family !== "Roboto") errors.push("tokens: screen font must be Roboto");
if (tokens?.budgets?.fullFrameBytes !== 8192) errors.push("tokens: mono1 frame must be 8192 bytes");
if (tokens?.interaction?.minHitWidth < 44 || tokens?.interaction?.minHitHeight < 44) {
  errors.push("tokens: hit target must be at least 44x44");
}

const iconIndex = readJson("icons/phosphor/ICON_INDEX.json");
const iconNames = new Set(iconIndex?.variants?.map((item) => item.name) || []);
for (const variant of iconIndex?.variants || []) {
  const relative = `icons/phosphor/${variant.name}-${variant.weight}.svg`;
  const absolute = path.join(root, relative);
  if (!fs.existsSync(absolute)) {
    errors.push(`missing icon variant: ${relative}`);
    continue;
  }
  const svg = fs.readFileSync(absolute, "utf8");
  if (!svg.startsWith("<svg") || !svg.includes('viewBox="0 0 256 256"')) {
    errors.push(`invalid icon source: ${relative}`);
  }
}
for (const name of manifest?.requiredIcons || []) {
  if (!iconNames.has(name)) errors.push(`missing required icon: ${name}`);
}

for (const relative of [
  "reference/screens/preview-square.png",
  "reference/screens/preview-round.png",
  "reference/screens/flow/01-devices-square.png",
  "reference/screens/events/blocking-round.png",
  "reference/screens/commands/success-square.png",
  "reference/screens/safety/approved-round.jpg"
]) {
  if (!fs.existsSync(path.join(root, relative))) errors.push(`missing reference: ${relative}`);
}

if (errors.length) {
  process.stderr.write(`Graphics package validation failed (${errors.length})\n`);
  for (const error of errors) process.stderr.write(`- ${error}\n`);
  process.exit(1);
}

process.stdout.write(
  `Graphics package validation passed\n` +
  `- targets: ${manifest.targets.length}\n` +
  `- required icons: ${manifest.requiredIcons.length}\n` +
  `- exported icon variants: ${iconIndex.variants.length}\n` +
  `- screen font: ${tokens.font.family}\n`
);
