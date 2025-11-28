"use strict";

import { isNitroSQLiteNull, isSimpleNullHandlingEnabled } from "../nullHandling.js";
import { HybridNitroSQLite } from "../nitro.js";
import { replaceWithNativeNullValue } from "../nullHandling.js";
import NitroSQLiteError from "../NitroSQLiteError.js";
export function execute(dbName, query, params) {
  const transformedParams = isSimpleNullHandlingEnabled() ? toNativeQueryParams(params) : params;
  try {
    const nativeResult = HybridNitroSQLite.execute(dbName, query, transformedParams);
    return buildJsQueryResult(nativeResult);
  } catch (error) {
    throw NitroSQLiteError.fromError(error);
  }
}
export async function executeAsync(dbName, query, params) {
  const transformedParams = isSimpleNullHandlingEnabled() ? toNativeQueryParams(params) : params;
  try {
    const nativeResult = await HybridNitroSQLite.executeAsync(dbName, query, transformedParams);
    return buildJsQueryResult(nativeResult);
  } catch (error) {
    throw NitroSQLiteError.fromError(error);
  }
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