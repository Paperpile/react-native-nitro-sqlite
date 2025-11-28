"use strict";

import { locks, HybridNitroSQLite } from "../nitro.js";
import { transaction } from "./transaction.js";
import { execute, executeAsync } from "./execute.js";
import { executeBatch, executeBatchAsync } from "./executeBatch.js";
export function open(options) {
  openDb(options.name, options.location);
  return {
    close: () => close(options.name),
    delete: () => HybridNitroSQLite.drop(options.name, options.location),
    attach: (dbNameToAttach, alias, location) => HybridNitroSQLite.attach(options.name, dbNameToAttach, alias, location),
    detach: alias => HybridNitroSQLite.detach(options.name, alias),
    transaction: fn => transaction(options.name, fn),
    execute: (query, params) => execute(options.name, query, params),
    executeAsync: (query, params) => executeAsync(options.name, query, params),
    executeBatch: commands => executeBatch(options.name, commands),
    executeBatchAsync: commands => executeBatchAsync(options.name, commands),
    loadFile: location => HybridNitroSQLite.loadFile(options.name, location),
    loadFileAsync: location => HybridNitroSQLite.loadFileAsync(options.name, location),
    loadExtension: (path, entryPoint) => HybridNitroSQLite.loadExtension(options.name, path, entryPoint)
  };
}
export function openDb(dbName, location) {
  HybridNitroSQLite.open(dbName, location);
  locks[dbName] = {
    queue: [],
    inProgress: false
  };
}
export function close(dbName) {
  HybridNitroSQLite.close(dbName);
  delete locks[dbName];
}
//# sourceMappingURL=session.js.map