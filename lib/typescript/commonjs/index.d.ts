import { open } from './operations/session';
import { execute, executeAsync } from './operations/execute';
import { executeBatch, executeBatchAsync } from './operations/executeBatch';
export type * from './types';
export { typeORMDriver } from './typeORM';
export declare const NitroSQLite: {
    native: import("./specs/NitroSQLite.nitro").NitroSQLite;
    open: typeof open;
    transaction: (dbName: string, fn: (tx: import("./types").Transaction) => Promise<void> | void) => Promise<void>;
    execute: typeof execute;
    executeAsync: typeof executeAsync;
    executeBatch: typeof executeBatch;
    executeBatchAsync: typeof executeBatchAsync;
    loadExtension: (dbName: string, path: string, entryPoint?: string) => void;
    close(dbName: string): void;
    drop(dbName: string, location?: string): void;
    attach(mainDbName: string, dbNameToAttach: string, alias: string, location?: string): void;
    detach(mainDbName: string, alias: string): void;
    loadFile(dbName: string, location: string): import("./types").FileLoadResult;
    loadFileAsync(dbName: string, location: string): Promise<import("./types").FileLoadResult>;
    __type?: string;
    name: string;
    toString(): string;
    equals(other: import("react-native-nitro-modules").HybridObject<{
        ios: "c++";
        android: "c++";
    }>): boolean;
    dispose(): void;
};
export { open } from './operations/session';
export { isNitroSQLiteNull, NITRO_SQLITE_NULL, isSimpleNullHandlingEnabled, enableSimpleNullHandling, } from './nullHandling';
//# sourceMappingURL=index.d.ts.map