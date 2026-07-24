import assert from "node:assert/strict";
import test from "node:test";
import { EnvironmentCredentialProvider, MacOSKeychainCredentialProvider } from "../src/credential-provider.js";

test("Keychain provider uses security without credential in arguments", async () => {
  let observed: readonly string[] = [];
  const provider = new MacOSKeychainCredentialProvider("service", "account", async (file, args) => {
    assert.equal(file, "/usr/bin/security"); observed = args; return { stdout: "secret-value\n" };
  });
  const material = await provider.load();
  assert.equal(material.token, "secret-value");
  assert.equal(observed.includes("secret-value"), false);
});

test("environment provider never names credential value in failure", async () => {
  const provider = new EnvironmentCredentialProvider("HOMEY_PRIVATE_TOKEN", {});
  await assert.rejects(provider.load(), /environment variable is unavailable/);
});
