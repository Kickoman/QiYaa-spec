// Checks jam/ beyond the protocol: the ordering cases are well formed, scenario IDs are unique and
// every ID mentioned exists, and relative links and their anchors resolve.
import { existsSync, readdirSync, readFileSync, statSync } from "node:fs";
import { dirname, join, relative } from "node:path";
import { fileURLToPath } from "node:url";
import Ajv2020Module from "ajv/dist/2020.js";

const Ajv2020 = Ajv2020Module.default ?? Ajv2020Module;
const spec = fileURLToPath(new URL("../", import.meta.url));
const jam = join(spec, "jam");
const failures = [];

function markdownFiles(dir) {
  return readdirSync(dir).flatMap((name) => {
    const path = join(dir, name);
    if (statSync(path).isDirectory()) return markdownFiles(path);
    return name.endsWith(".md") ? [path] : [];
  });
}

const item = {
  type: "object",
  required: ["itemId", "addedBy", "addedAt", "pinnedAt"],
  additionalProperties: false,
  properties: {
    itemId: { type: "string", pattern: "^i[1-9][0-9]*$" },
    addedBy: { type: "string", minLength: 1 },
    addedAt: { type: "integer", minimum: 0 },
    pinnedAt: { anyOf: [{ type: "integer", minimum: 0 }, { type: "null" }] },
  },
};
const orderingCase = new Ajv2020({ strict: true, allErrors: true }).compile({
  type: "object",
  required: ["name", "mode", "items", "lastServedAt", "expected"],
  additionalProperties: false,
  properties: {
    name: { type: "string", minLength: 1 },
    mode: { enum: ["round-robin", "fifo"] },
    items: { type: "array", items: item },
    lastServedAt: { type: "object", additionalProperties: { type: "integer", minimum: 0 } },
    expected: { type: "array", items: { type: "string" } },
  },
});

const orderingDir = join(jam, "ordering");
let cases = 0;
for (const name of readdirSync(orderingDir).filter((file) => file.endsWith(".json"))) {
  const path = join(orderingDir, name);
  const text = readFileSync(path, "utf8");
  const data = JSON.parse(text);
  cases++;
  if (JSON.stringify(data, null, 2) + "\n" !== text) failures.push(`ordering/${name}: not formatted with 2-space indentation`);
  if (!orderingCase(data)) {
    failures.push(`ordering/${name}: ${JSON.stringify(orderingCase.errors)}`);
    continue;
  }
  const ids = data.items.map((entry) => entry.itemId);
  if (new Set(ids).size !== ids.length) failures.push(`ordering/${name}: duplicate itemId`);
  if ([...ids].sort().join() !== [...data.expected].sort().join()) {
    failures.push(`ordering/${name}: expected is not every item exactly once`);
  }
}

const idPattern = /\b(ROOM|REC|SEED|HOST)-\d{2}\b/g;
const playerPattern = /\b(ERR|TRK|TR|WAVE|SRC)-\d{2}\b/g;
const defined = new Map();
const playerDefined = new Set();
for (const path of markdownFiles(join(spec, "player"))) {
  for (const match of readFileSync(path, "utf8").matchAll(/^\| ((?:ERR|TRK|TR|WAVE|SRC)-\d{2}) \|/gm)) playerDefined.add(match[1]);
}

const docs = markdownFiles(jam);
for (const path of docs) {
  for (const match of readFileSync(path, "utf8").matchAll(/^\| ((?:ROOM|REC|SEED|HOST)-\d{2}) \|/gm)) {
    const where = relative(jam, path);
    if (defined.has(match[1])) failures.push(`${match[1]}: defined in ${defined.get(match[1])} and ${where}`);
    defined.set(match[1], where);
  }
}

function slug(heading) {
  return heading.trim().toLowerCase().replace(/[^\p{L}\p{N} _-]/gu, "").replace(/ /g, "-");
}

function anchors(path) {
  const text = readFileSync(path, "utf8").replace(/```[\s\S]*?```/g, "");
  return new Set([...text.matchAll(/^#{1,6} (.+)$/gm)].map((match) => slug(match[1])));
}

for (const path of docs) {
  const where = relative(jam, path);
  const text = readFileSync(path, "utf8").replace(/```[\s\S]*?```/g, "");
  for (const match of text.matchAll(idPattern)) {
    if (!defined.has(match[0])) failures.push(`${where}: mentions ${match[0]}, which is not defined`);
  }
  for (const match of text.matchAll(playerPattern)) {
    if (!playerDefined.has(match[0])) failures.push(`${where}: mentions ${match[0]}, which is not in player/`);
  }
  for (const match of text.matchAll(/\]\(([^)\s]+)\)/g)) {
    const link = match[1];
    if (/^[a-z]+:/.test(link)) continue;
    const [file, anchor] = link.split("#");
    const target = file ? join(dirname(path), file) : path;
    if (!existsSync(target)) {
      failures.push(`${where}: link ${link} points to nothing`);
      continue;
    }
    const page = statSync(target).isDirectory() ? join(target, "README.md") : target;
    if (anchor && (!page.endsWith(".md") || !anchors(page).has(anchor))) {
      failures.push(`${where}: link ${link} has no such heading`);
    }
  }
}

for (const prefix of ["ROOM", "REC", "SEED", "HOST"]) {
  const numbers = [...defined.keys()].filter((id) => id.startsWith(`${prefix}-`)).map((id) => Number(id.slice(-2))).sort((a, b) => a - b);
  numbers.forEach((number, index) => {
    if (number !== index + 1) failures.push(`${prefix}-: IDs are not 01 to ${numbers.length} without gaps`);
  });
}

if (failures.length) {
  console.error([...new Set(failures)].join("\n"));
  process.exit(1);
}
console.log(`jam scenarios: ${defined.size} IDs, ${cases} ordering cases, links resolve`);
