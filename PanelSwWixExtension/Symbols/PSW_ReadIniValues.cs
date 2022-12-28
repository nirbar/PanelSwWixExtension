using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_ReadIniValues : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_ReadIniValues), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_ReadIniValues));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(FilePath), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Section), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Key), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(DestProperty), ColumnType.String, 0, false, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(Attributes), ColumnType.Number, 2, false, false, ColumnCategory.Integer, minValue: 0, maxValue: 127),
                    new ColumnDefinition(nameof(Condition), ColumnType.String, 0, false, true, ColumnCategory.Condition, modularizeType: ColumnModularizeType.Condition),
                };
            }
        }

        public PSW_ReadIniValues() : base(SymbolDefinition)
        { }

        public PSW_ReadIniValues(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "ini")
        { }

        public string FilePath
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public string Section
        {
            get => Fields[1].AsString();
            set => this.Set(1, value);
        }

        public string Key
        {
            get => Fields[2].AsString();
            set => this.Set(2, value);
        }

        public string DestProperty
        {
            get => Fields[3].AsString();
            set => this.Set(3, value);
        }

        public int Attributes
        {
            get => Fields[4].AsNumber();
            set => this.Set(4, value);
        }

        public string Condition
        {
            get => Fields[5].AsString();
            set => this.Set(5, value);
        }
    }
}
