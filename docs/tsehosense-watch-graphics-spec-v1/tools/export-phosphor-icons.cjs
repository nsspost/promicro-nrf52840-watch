#!/usr/bin/env node
"use strict";

/*
 * Exports the exact Phosphor sources used by the accepted prototype.
 * Usage:
 *   node tools/export-phosphor-icons.cjs \
 *     ../strict-context-watchface/node_modules \
 *     icons/phosphor
 */

const fs = require("fs");
const path = require("path");
const { pathToFileURL } = require("url");

const nodeModules = path.resolve(process.argv[2] || "node_modules");
const output = path.resolve(process.argv[3] || "icons/phosphor");
const React = require(path.join(nodeModules, "react"));
const ReactDOMServer = require(path.join(nodeModules, "react-dom/server"));

const regular = [
  "ArrowClockwise", "ArrowSquareOut", "BatteryMedium", "BellSimple",
  "Broadcast", "CaretLeft", "CaretRight", "ChatCircle", "CheckCircle",
  "CircleNotch", "Cloud", "Gauge", "HandTap", "Hexagon", "Info",
  "ListBullets", "LockKey", "MapPin", "Minus", "Plus", "Power", "Radio",
  "ShieldWarning", "SlidersHorizontal", "StopCircle", "UserSwitch",
  "Warning", "WarningOctagon", "Waveform", "WifiSlash", "X", "XCircle"
];

const variants = [
  ...regular.map((name) => ({ name, weight: "regular" })),
  { name: "CaretRight", weight: "bold" },
  { name: "Broadcast", weight: "fill" },
  { name: "CheckCircle", weight: "fill" }
];

(async () => {
  const entry = path.join(nodeModules, "@phosphor-icons/react/dist/index.es.js");
  const Phosphor = await import(pathToFileURL(entry).href);

  fs.mkdirSync(output, { recursive: true });

  for (const { name, weight } of variants) {
    const Icon = Phosphor[name];
    if (!Icon) throw new Error(`Missing Phosphor icon: ${name}`);
    const markup = ReactDOMServer.renderToStaticMarkup(
      React.createElement(Icon, {
        size: 256,
        weight,
        color: "currentColor",
        "aria-hidden": "true"
      })
    );
    const filename = `${name}-${weight}.svg`;
    fs.writeFileSync(path.join(output, filename), `${markup}\n`, "utf8");
  }

  fs.writeFileSync(
    path.join(output, "ICON_INDEX.json"),
    `${JSON.stringify({ source: "@phosphor-icons/react", viewBox: "0 0 256 256", variants }, null, 2)}\n`,
    "utf8"
  );
})().catch((error) => {
  process.stderr.write(`${error.stack || error}\n`);
  process.exitCode = 1;
});
