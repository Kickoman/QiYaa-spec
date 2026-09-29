// Validates jam/protocol: the schemas compile, every example passes or fails as its name says,
// every invalid example is explained in examples/README.md, and every JSON file is formatted
// as the spec requires (2-space indentation, trailing newline).
import { readdirSync, readFileSync, statSync } from "node:fs";
import { join, relative } from "node:path";
import { fileURLToPath } from "node:url";
import Ajv2020Module from "ajv/dist/2020.js";

const Ajv2020 = Ajv2020Module.default ?? Ajv2020Module;
const root = fileURLToPath(new URL("../jam/protocol/", import.meta.url));
const schemaDir = join(root, "schemas");
const exampleDir = join(root, "examples");
const failures = [];

function files(dir) {
  return readdirSync(dir).flatMap((name) => {
    const path = join(dir, name);
    return statSync(path).isDirectory() ? files(path) : [path];
  });
}

function readJson(path) {
  const text = readFileSync(path, "utf8");
  const data = JSON.parse(text);
  if (JSON.stringify(data, null, 2) + "\n" !== text) {
    failures.push(`${relative(root, path)}: not formatted with 2-space indentation and a trailing newline`);
  }
  return data;
}

const ajv = new Ajv2020({ strict: true, strictRequired: false, allErrors: true });
const schemas = files(schemaDir).filter((path) => path.endsWith(".schema.json"));
for (const path of schemas) ajv.addSchema(readJson(path));

const messageIds = new Map();
for (const path of schemas) {
  const schema = readJson(path);
  ajv.getSchema(schema.$id);
  const match = relative(schemaDir, path).match(/^messages\/(.+)\.schema\.json$/);
  if (match) messageIds.set(match[1], schema.$id);
}

const idOf = (name) => schemas.map(readJson).find((schema) => schema.$id.endsWith(`/${name}`)).$id;
const anyClient = ajv.getSchema(idOf("client-message.schema.json"));
const anyServer = ajv.getSchema(idOf("server-message.schema.json"));
const explained = readFileSync(join(exampleDir, "README.md"), "utf8");

let checked = 0;
for (const [type, id] of messageIds) {
  const validate = ajv.getSchema(id);
  let dir;
  try {
    dir = readdirSync(join(exampleDir, type));
  } catch {
    failures.push(`${type}: no examples`);
    continue;
  }
  if (!dir.some((name) => !name.startsWith("invalid-"))) failures.push(`${type}: no valid example`);
  for (const name of dir) {
    const path = join(exampleDir, type, name);
    const shown = relative(root, path);
    const message = readJson(path);
    const valid = validate(message);
    checked++;
    if (name.startsWith("invalid-")) {
      if (valid) failures.push(`${shown}: expected to fail, but passes`);
      if (!explained.includes(`\`${type}/${name}\``)) failures.push(`${shown}: not explained in examples/README.md`);
    } else {
      if (!valid) failures.push(`${shown}: ${ajv.errorsText(validate.errors)}`);
      if (message.type !== type) failures.push(`${shown}: "type" is ${JSON.stringify(message.type)}, not "${type}"`);
      if (!anyClient(message) && !anyServer(message)) failures.push(`${shown}: matches neither direction`);
    }
  }
}

for (const name of readdirSync(exampleDir)) {
  if (name !== "README.md" && !messageIds.has(name)) failures.push(`examples/${name}: no schema for this type`);
}

if (failures.length) {
  console.error(failures.join("\n"));
  process.exit(1);
}
console.log(`jam/protocol: ${schemas.length} schemas, ${checked} examples, all as expected`);
