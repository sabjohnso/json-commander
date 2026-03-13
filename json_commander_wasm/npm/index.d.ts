export interface ValidationError {
  path?: string;
  message: string;
}

export interface ErrorResult {
  success: false;
  errors: ValidationError[];
}

export interface SuccessResult {
  success: true;
}

export interface TextResult {
  success: true;
  text: string;
}

export interface ConfigResult {
  success: true;
  config: Record<string, unknown>;
}

export interface SchemaResult {
  success: true;
  schema: Record<string, unknown>;
}

export type ValidateResult = SuccessResult | ErrorResult;
export type HelpResult = TextResult | ErrorResult;
export type ManpageResult = TextResult | ErrorResult;
export type ConfigSchemaResult = SchemaResult | ErrorResult;
export type ParseResult = ConfigResult | ErrorResult;

export function init(): Promise<void>;
export function validateSchema(schemaJson: string): ValidateResult;
export function generateHelp(schemaJson: string, commandPath?: string[]): HelpResult;
export function generateManpage(schemaJson: string, commandPath?: string[]): ManpageResult;
export function generateConfigSchema(schemaJson: string, commandPath?: string[]): ConfigSchemaResult;
export function parseArgs(schemaJson: string, argv: string[], env?: Record<string, string>): ParseResult;
