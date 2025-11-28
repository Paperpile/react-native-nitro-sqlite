"use strict";

import { isSimpleNullHandlingEnabled, replaceWithNativeNullValue } from "../nullHandling.js";
import { HybridNitroSQLite } from "../nitro.js";
import { queueOperationAsync, startOperationSync, throwIfDatabaseIsNotOpen } from "../DatabaseQueue.js";
import NitroSQLiteError from "../NitroSQLiteError.js";
export function executeBatch(dbName, commands) {
  throwIfDatabaseIsNotOpen(dbName);
  const transformedCommands = isSimpleNullHandlingEnabled() ? toNativeBatchQueryCommands(commands) : commands;
  try {
    return startOperationSync(dbName, () => HybridNitroSQLite.executeBatch(dbName, transformedCommands));
  } catch (error) {
    throw NitroSQLiteError.fromError(error);
  }
}
export async function executeBatchAsync(dbName, commands) {
  throwIfDatabaseIsNotOpen(dbName);
  const transformedCommands = isSimpleNullHandlingEnabled() ? toNativeBatchQueryCommands(commands) : commands;
  return queueOperationAsync(dbName, async () => {
    try {
      return await HybridNitroSQLite.executeBatchAsync(dbName, transformedCommands);
    } catch (error) {
      throw NitroSQLiteError.fromError(error);
    }
  });
}
function toNativeBatchQueryCommands(commands) {
  return commands.map(command => {
    const transformedParams = command.params?.map(param => {
      if (Array.isArray(param)) {
        return param.map(p => replaceWithNativeNullValue(p));
      }
      return replaceWithNativeNullValue(param);
    });
    return {
      query: command.query,
      params: transformedParams
    };
  });
}
//# sourceMappingURL=executeBatch.js.map