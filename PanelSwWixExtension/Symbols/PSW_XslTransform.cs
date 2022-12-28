using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_XslTransform : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_XslTransform), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_XslTransform));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(File_), ColumnType.String, 72, false, true, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "File", keyColumn: 1),
                    new ColumnDefinition(nameof(Component_), ColumnType.String, 72, false, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "Component", keyColumn: 1),
                    new ColumnDefinition(nameof(FilePath), ColumnType.Localized, 0, false, true, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(XslBinary_), ColumnType.String, 72, false, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "Binary", keyColumn: 1),
                    new ColumnDefinition(nameof(Order), ColumnType.Number, 4, false, false, ColumnCategory.Integer, minValue: 0, maxValue: int.MaxValue),
                    new ColumnDefinition(nameof(On), ColumnType.Number, 2, false, false, ColumnCategory.Integer, minValue: 0, maxValue: 127),
                };
            }
        }

        public PSW_XslTransform() : base(SymbolDefinition)
        { }

        public PSW_XslTransform(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "xsl")
        { }

        public string File_
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public string Component_
        {
            get => Fields[1].AsString();
            set => this.Set(1, value);
        }

        public string FilePath
        {
            get => Fields[2].AsString();
            set => this.Set(2, value);
        }

        public string XslBinary_
        {
            get => Fields[3].AsString();
            set => this.Set(3, value);
        }

        public int Order
        {
            get => Fields[4].AsNumber();
            set => this.Set(4, value);
        }

        public int On
        {
            get => Fields[4].AsNumber();
            set => this.Set(4, value);
        }
    }
}
