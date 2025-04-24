"use strict";

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.executeBatch = executeBatch;
exports.executeBatchAsync = executeBatchAsync;
var _nullHandling = require("../nullHandling.js");
var _nitro = require("../nitro.js");
function executeBatch(dbName, commands) {
  const transformedCommands = (0, _nullHandling.isSimpleNullHandlingEnabled)() ? toNativeBatchQueryCommands(commands) : commands;
  const result = _nitro.HybridNitroSQLite.executeBatch(dbName, transformedCommands);
  return result;
}
async function executeBatchAsync(dbName, commands) {
  const transformedCommands = (0, _nullHandling.isSimpleNullHandlingEnabled)() ? toNativeBatchQueryCommands(commands) : commands;
  const result = await _nitro.HybridNitroSQLite.executeBatchAsync(dbName, transformedCommands);
  return result;
}
function toNativeBatchQueryCommands(commands) {
  return commands.map(command => {
    const transformedParams = command.params?.map(param => {
      if (Array.isArray(param)) {
        return param.map(p => (0, _nullHandling.replaceWithNativeNullValue)(p));
      }
      return (0, _nullHandling.replaceWithNativeNullValue)(param);
    });
    return {
      query: command.query,
      params: transformedParams
    };
  });
}
//# sourceMappingURL=executeBatch.js.map