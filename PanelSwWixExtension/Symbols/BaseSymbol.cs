using System;
using System.Collections.Generic;
using System.Linq;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal abstract class BaseSymbol : IntermediateSymbol
    {
        public BaseSymbol(IntermediateSymbolDefinition definition) : base(definition) { }

        public BaseSymbol(IntermediateSymbolDefinition definition, SourceLineNumber lineNumber, Identifier id) : base(definition, lineNumber, id) { }

        public BaseSymbol(IntermediateSymbolDefinition definition, SourceLineNumber lineNumber, string idPrefix) : base(definition, lineNumber, new Identifier(AccessModifier.Global, $"{idPrefix}{Guid.NewGuid().ToString("N")}")) { }

        protected static IntermediateFieldDefinition[] CreateFieldDefinitions(IEnumerable<ColumnDefinition> columns, int firstColumn = 1 /* Skip the Id column for most tables */)
        {
            List<IntermediateFieldDefinition> fieldDefinitions = new List<IntermediateFieldDefinition>();
            for (int i = firstColumn; i < columns.Count(); ++i)
            {
                ColumnDefinition column = columns.ElementAt(i);
                IntermediateFieldType fieldType = IntermediateFieldType.String;
                switch (column.Type)
                {
                    case ColumnType.String:
                    case ColumnType.Localized:
                    case ColumnType.Preserved:
                        switch (column.Category)
                        {
                            case ColumnCategory.AnyPath:
                            case ColumnCategory.RegPath:
                            case ColumnCategory.Path:
                                fieldType = IntermediateFieldType.Path;
                                break;

                            case ColumnCategory.Condition:
                            case ColumnCategory.DefaultDir:
                            case ColumnCategory.Filename:
                            case ColumnCategory.Formatted:
                            case ColumnCategory.FormattedSDDLText:
                            case ColumnCategory.Guid:
                            case ColumnCategory.Paths:
                            case ColumnCategory.Identifier:
                            case ColumnCategory.LowerCase:
                            case ColumnCategory.Property:
                            case ColumnCategory.Shortcut:
                            case ColumnCategory.Text:
                            case ColumnCategory.TimeDate:
                            case ColumnCategory.UpperCase:
                            case ColumnCategory.Version:
                            case ColumnCategory.WildCardFilename:
                                fieldType = IntermediateFieldType.String;
                                break;
                            default:
                                throw new InvalidProgramException("Do not call 'CreateFieldDefinitions' for columns with strange field types");
                        }
                        break;
                    case ColumnType.Number:
                        fieldType = IntermediateFieldType.Number;
                        break;
                    default:
                        throw new InvalidProgramException("Do not call 'CreateFieldDefinitions' for columns with strange field types");
                }
                fieldDefinitions.Add(new IntermediateFieldDefinition(column.Name, fieldType));
            }
            return fieldDefinitions.ToArray();
        }

        public bool LoadFromRow(Row row, out string id)
        {
            id = null;
            TableDefinition definition = PanelSwWixExtension.TableDefinitions.FirstOrDefault(td => td.Name.Equals(GetType().Name));
            if (definition == null)
            {
                return false;
            }

            int offset = definition.SymbolIdIsPrimaryKey ? 1 : 0;
            if (definition.SymbolIdIsPrimaryKey)
            {
                offset = 1;
                id = row[0]?.ToString();
            }

            for (int i = offset; i < definition.Columns.Count(); ++i)
            {
                this.Set(i - offset, row[i]?.ToString());
            }
            return true;
        }
    }
}
