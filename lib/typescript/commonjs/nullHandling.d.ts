import { SQLiteNullValue, SQLiteValue } from './types';
import { SQLiteQueryParamItem } from './types';
export declare function enableSimpleNullHandling(shouldEnableSimpleNullHandling?: boolean): void;
export declare function isSimpleNullHandlingEnabled(): boolean;
export declare const NITRO_SQLITE_NULL: SQLiteNullValue;
export declare function isNitroSQLiteNull(value: unknown): value is SQLiteNullValue;
export declare function replaceWithNativeNullValue(value: SQLiteQueryParamItem): SQLiteValue;
//# sourceMappingURL=nullHandling.d.ts.map