"use strict";

import { isSimpleNullHandlingEnabled, replaceWithNativeNullValue } from "../nullHandling.js";
import { HybridNitroSQLite } from "../nitro.js";
export function executeBatch(dbName, commands) {
  const transformedCommands = isSimpleNullHandlingEnabled() ? toNativeBatchQueryCommands(commands) : commands;
  const result = HybridNitroSQLite.executeBatch(dbName, transformedCommands);
  return result;
}
export async function executeBatchAsync(dbName, commands) {
  const transformedCommands = isSimpleNullHandlingEnabled() ? toNativeBatchQueryCommands(commands) : commands;
  const result = await HybridNitroSQLite.executeBatchAsync(dbName, transformedCommands);
  return result;
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