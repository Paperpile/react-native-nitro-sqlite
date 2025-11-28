"use strict";

Object.defineProperty(exports, "__esModule", {
  value: true
});
Object.defineProperty(exports, "NITRO_SQLITE_NULL", {
  enumerable: true,
  get: function () {
    return _nullHandling.NITRO_SQLITE_NULL;
  }
});
exports.NitroSQLite = void 0;
Object.defineProperty(exports, "NitroSQLiteError", {
  enumerable: true,
  get: function () {
    return _NitroSQLiteError.default;
  }
});
Object.defineProperty(exports, "enableSimpleNullHandling", {
  enumerable: true,
  get: function () {
    return _nullHandling.enableSimpleNullHandling;
  }
});
Object.defineProperty(exports, "isNitroSQLiteNull", {
  enumerable: true,
  get: function () {
    return _nullHandling.isNitroSQLiteNull;
  }
});
Object.defineProperty(exports, "isSimpleNullHandlingEnabled", {
  enumerable: true,
  get: function () {
    return _nullHandling.isSimpleNullHandlingEnabled;
  }
});
Object.defineProperty(exports, "open", {
  enumerable: true,
  get: function () {
    return _session.open;
  }
});
Object.defineProperty(exports, "typeORMDriver", {
  enumerable: true,
  get: function () {
    return _typeORM.typeORMDriver;
  }
});
var _transaction = require("./operations/transaction.js");
var _nitro = require("./nitro.js");
var _session = require("./operations/session.js");
var _execute = require("./operations/execute.js");
var _OnLoad = require("./OnLoad");
var _executeBatch = require("./operations/executeBatch.js");
var _nullHandling = require("./nullHandling.js");
var _NitroSQLiteError = _interopRequireDefault(require("./NitroSQLiteError.js"));
var _typeORM = require("./typeORM.js");
function _interopRequireDefault(e) { return e && e.__esModule ? e : { default: e }; }
(0, _OnLoad.init)();
const NitroSQLite = exports.NitroSQLite = {
  ..._nitro.HybridNitroSQLite,
  native: _nitro.HybridNitroSQLite,
  // Overwrite native `open` function with session-based JS abstraction,
  // where the database name can be ommited once opened
  open: _session.open,
  // More JS abstractions, that perform type casting and validation.
  transaction: _transaction.transaction,
  execute: _execute.execute,
  executeAsync: _execute.executeAsync,
  executeBatch: _executeBatch.executeBatch,
  executeBatchAsync: _executeBatch.executeBatchAsync,
  // Add loadExtension to make it accessible from the NitroSQLite object
  loadExtension: _nitro.HybridNitroSQLite.loadExtension
};
//# sourceMappingURL=index.js.map