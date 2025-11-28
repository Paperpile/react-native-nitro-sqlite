"use strict";

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.execute = execute;
exports.executeAsync = executeAsync;
var _nullHandling = require("../nullHandling.js");
var _nitro = require("../nitro.js");
var _NitroSQLiteError = _interopRequireDefault(require("../NitroSQLiteError.js"));
function _interopRequireDefault(e) { return e && e.__esModule ? e : { default: e }; }
function execute(dbName, query, params) {
  const transformedParams = (0, _nullHandling.isSimpleNullHandlingEnabled)() ? toNativeQueryParams(params) : params;
  try {
    const nativeResult = _nitro.HybridNitroSQLite.execute(dbName, query, transformedParams);
    return buildJsQueryResult(nativeResult);
  } catch (error) {
    throw _NitroSQLiteError.default.fromError(error);
  }
}
async function executeAsync(dbName, query, params) {
  const transformedParams = (0, _nullHandling.isSimpleNullHandlingEnabled)() ? toNativeQueryParams(params) : params;
  try {
    const nativeResult = await _nitro.HybridNitroSQLite.executeAsync(dbName, query, transformedParams);
    return buildJsQueryResult(nativeResult);
  } catch (error) {
    throw _NitroSQLiteError.default.fromError(error);
  }
}
function toNativeQueryParams(params) {
  return params?.map(param => (0, _nullHandling.replaceWithNativeNullValue)(param));
}
function buildJsQueryResult({
  insertId,
  rowsAffected,
  results
}) {
  let data = results;
  if ((0, _nullHandling.isSimpleNullHandlingEnabled)()) {
    data = results.map(row => Object.fromEntries(Object.entries(row).map(([key, value]) => {
      if ((0, _nullHandling.isNitroSQLiteNull)(value)) {
        return [key, null];
      }
      return [key, value];
    })));
  }
  return {
    insertId,
    rowsAffected,
    rows: {
      _array: data,
      length: data.length,
      item: idx => data[idx]
    }
  };
}
//# sourceMappingURL=execute.js.map