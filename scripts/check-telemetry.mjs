// Validates telemetry/: the schema compiles, every example passes or fails as its name says,
// every invalid example is explained in examples/README.md, every JSON file is formatted as the
// spec requires, and the TEL- scenario IDs are unique.
import { readdirSync, readFileSync } from "node:fs";
import { join } from "node:path";
import { fileURLToPath } from "node:url";
import Ajv2020Module from "ajv/dist/2020.js";

const Ajv2020 = Ajv2020Module.default ?? Ajv2020Module;
const root = fileURLToPath(new URL("../telemetry/", import.meta.url));
const failures = [];

function readJson(path, shown) {
  const text = readFileSync(path, "utf8");
  const data = JSON.parse(text);
  if (JSON.stringify(data, null, 2) + "\n" !== text) {
    failures.push(`${shown}: not formatted with 2-space indentation and a trailing newline`);
  }
  return data;
}

const ajv = new Ajv2020({ strict: true, strictRequired: false, allErrors: true });
const validate = ajv.compile(readJson(join(root, "schemas/batch.schema.json"), "schemas/batch.schema.json"));
const exampleDir = join(root, "examples");
const explained = readFileSync(join(exampleDir, "README.md"), "utf8");

let checked = 0;
for (const name of readdirSync(exampleDir).filter((file) => file.endsWith(".json"))) {
  const shown = `examples/${name}`;
  const valid = validate(readJson(join(exampleDir, name), shown));
  checked++;
  if (name.startsWith("invalid-")) {
    if (valid) failures.push(`${shown}: expected to fail, but passes`);
    if (!explained.includes(`\`${name}\``)) failures.push(`${shown}: not explained in examples/README.md`);
  } else if (!valid) {
    failures.push(`${shown}: ${ajv.errorsText(validate.errors)}`);
  }
}

const ids = [...readFileSync(join(root, "README.md"), "utf8").matchAll(/^\| (TEL-\d{2}) \|/gm)].map((m) => m[1]);
for (const id of ids.filter((id, index) => ids.indexOf(id) !== index)) failures.push(`${id}: defined twice`);

if (failures.length) {
  console.error(failures.join("\n"));
  process.exit(1);
}
console.log(`telemetry: ${checked} examples, ${ids.length} scenario IDs, all as expected`);
