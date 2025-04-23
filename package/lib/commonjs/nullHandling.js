"use strict";

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.NITRO_SQLITE_NULL = void 0;
exports.enableSimpleNullHandling = enableSimpleNullHandling;
exports.isNitroSQLiteNull = isNitroSQLiteNull;
exports.isSimpleNullHandlingEnabled = isSimpleNullHandlingEnabled;
exports.replaceWithNativeNullValue = replaceWithNativeNullValue;
let ENABLE_SIMPLE_NULL_HANDLING = false;
function enableSimpleNullHandling(shouldEnableSimpleNullHandling = true) {
  ENABLE_SIMPLE_NULL_HANDLING = shouldEnableSimpleNullHandling;
}
function isSimpleNullHandlingEnabled() {
  return ENABLE_SIMPLE_NULL_HANDLING;
}
const NITRO_SQLITE_NULL = exports.NITRO_SQLITE_NULL = {
  isNitroSQLiteNull: true
};
function isNitroSQLiteNull(value) {
  if (typeof value === 'object' && 'isNitroSQLiteNull' in value) {
    return true;
  }
  return false;
}
function replaceWithNativeNullValue(value) {
  if (value === undefined || value === null) {
    return NITRO_SQLITE_NULL;
  }
  return value;
}
//# sourceMappingURL=nullHandling.js.map