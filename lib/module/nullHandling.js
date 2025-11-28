"use strict";

let ENABLE_SIMPLE_NULL_HANDLING = false;
export function enableSimpleNullHandling(shouldEnableSimpleNullHandling = true) {
  ENABLE_SIMPLE_NULL_HANDLING = shouldEnableSimpleNullHandling;
}
export function isSimpleNullHandlingEnabled() {
  return ENABLE_SIMPLE_NULL_HANDLING;
}
export const NITRO_SQLITE_NULL = {
  isNitroSQLiteNull: true
};
export function isNitroSQLiteNull(value) {
  if (typeof value === 'object' && 'isNitroSQLiteNull' in value) {
    return true;
  }
  return false;
}
export function replaceWithNativeNullValue(value) {
  if (value === undefined || value === null) {
    return NITRO_SQLITE_NULL;
  }
  return value;
}
//# sourceMappingURL=nullHandling.js.map