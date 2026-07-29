import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";

const vectorUrl = new URL("./vectors.json", import.meta.url);
const document = JSON.parse(await readFile(vectorUrl, "utf8"));
const { major, minor, headerSize, maxPayload } = document.protocol;

function fromHex(value) {
  assert.equal(value.length % 2, 0, "hex strings must have even length");
  return Uint8Array.from(Buffer.from(value, "hex"));
}

function toHex(value) {
  return Buffer.from(value).toString("hex");
}

function encode(vector) {
  const payload = fromHex(vector.payloadHex);
  assert.ok(payload.length <= maxPayload);

  const frame = new Uint8Array(headerSize + payload.length);
  const view = new DataView(frame.buffer);
  frame[0] = 0x54;
  frame[1] = 0x4c;
  frame[2] = major;
  frame[3] = minor;
  frame[4] = vector.type;
  frame[5] = vector.flags;
  view.setUint16(6, vector.sequence, true);
  view.setUint16(8, payload.length, true);
  frame.set(payload, headerSize);
  return frame;
}

class Parser {
  constructor() {
    this.buffer = new Uint8Array(headerSize + maxPayload);
    this.used = 0;
    this.expected = headerSize;
    this.accepted = [];
    this.rejectedHeaders = 0;
    this.discardedBytes = 0;
  }

  reset() {
    this.used = 0;
    this.expected = headerSize;
  }

  feed(chunk) {
    for (const value of chunk) {
      if (this.used === 0) {
        if (value === 0x54) {
          this.buffer[0] = value;
          this.used = 1;
        } else {
          this.discardedBytes++;
        }
        continue;
      }

      if (this.used === 1) {
        if (value === 0x4c) {
          this.buffer[1] = value;
          this.used = 2;
        } else if (value === 0x54) {
          this.discardedBytes++;
          this.buffer[0] = value;
        } else {
          this.discardedBytes += 2;
          this.reset();
        }
        continue;
      }

      this.buffer[this.used++] = value;
      if (this.used === headerSize) {
        const payloadLength =
          this.buffer[8] | (this.buffer[9] << 8);
        if (payloadLength > maxPayload) {
          this.rejectedHeaders++;
          this.discardedBytes += headerSize;
          this.reset();
          continue;
        }
        this.expected = headerSize + payloadLength;
      }

      if (this.used === this.expected) {
        this.accepted.push(this.buffer.slice(0, this.expected));
        this.reset();
      }
    }
  }
}

for (const vector of document.vectors) {
  const encoded = encode(vector);
  assert.equal(toHex(encoded), vector.frameHex, vector.id);

  for (let split = 0; split <= encoded.length; split++) {
    const parser = new Parser();
    parser.feed(encoded.slice(0, split));
    parser.feed(encoded.slice(split));
    assert.equal(parser.accepted.length, 1, `${vector.id} split ${split}`);
    assert.equal(toHex(parser.accepted[0]), vector.frameHex);
  }
}

const parser = new Parser();
parser.feed(fromHex("005454004c"));
for (const vector of document.vectors) {
  parser.feed(fromHex(vector.frameHex));
}
assert.equal(parser.accepted.length, document.vectors.length);
assert.ok(parser.discardedBytes >= 5);

const oversizedHeader = fromHex("544c0001300001008101");
parser.feed(oversizedHeader);
assert.equal(parser.rejectedHeaders, 1);

const maxPayloadVector = {
  type: 0x30,
  flags: 0,
  sequence: 0xffff,
  payloadHex: "a5".repeat(maxPayload)
};
const maxFrame = encode(maxPayloadVector);
const maxParser = new Parser();
for (const byte of maxFrame) {
  maxParser.feed(Uint8Array.of(byte));
}
assert.equal(maxParser.accepted.length, 1);
assert.equal(maxParser.accepted[0].length, headerSize + maxPayload);

console.log(
  `Tseho Link vectors passed: ${document.vectors.length} golden frames, ` +
  `${maxFrame.length}-byte maximum frame`
);
