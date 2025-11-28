"use strict";

import { isNitroSQLiteNull, isSimpleNullHandlingEnabled } from "../nullHandling.js";
import { HybridNitroSQLite } from "../nitro.js";
import { replaceWithNativeNullValue } from "../nullHandling.js";
export function execute(dbName, query, params) {
  const transformedParams = isSimpleNullHandlingEnabled() ? toNativeQueryParams(params) : params;
  const nativeResult = HybridNitroSQLite.execute(dbName, query, transformedParams);
  const result = buildJsQueryResult(nativeResult);
  return result;
}
export async function executeAsync(dbName, query, params) {
  const transformedParams = isSimpleNullHandlingEnabled() ? toNativeQueryParams(params) : params;
  const nativeResult = await HybridNitroSQLite.executeAsync(dbName, query, transformedParams);
  const result = buildJsQueryResult(nativeResult);
  return result;
}
function toNativeQueryParams(params) {
  return params?.map(param => replaceWithNativeNullValue(param));
}
function buildJsQueryResult({
  insertId,
  rowsAffected,
  results
}) {
  let data = results;
  if (isSimpleNullHandlingEnabled()) {
    data = results.map(row => Object.fromEntries(Object.entries(row).map(([key, value]) => {
      if (isNitroSQLiteNull(value)) {
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