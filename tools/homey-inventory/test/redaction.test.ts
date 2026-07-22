import test from "node:test";
import assert from "node:assert/strict";
import { redact } from "../src/redaction.js";

test("redaction removes tokens, IPs, MACs, and URLs", () => {
  const token = ["Bearer", "abc.def"].join(" ");
  const ip = ["192", "168", "1", "10"].join(".");
  const mac = ["aa", "bb", "cc", "dd", "ee", "ff"].join(":");
  const url = ["https:", "", "private.example", "path"].join("/");
  const input = [token, ip, mac, url].join(" ");
  const output = redact(input);
  assert.equal(output.includes("abc.def"), false);
  assert.equal(output.includes(ip), false);
  assert.equal(output.includes(mac), false);
  assert.equal(output.includes("private.example"), false);
});
