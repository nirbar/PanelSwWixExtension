using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_DeletePath : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_DeletePath), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_DeletePath));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(Path), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Flags), ColumnType.Number, 2, false, false, ColumnCategory.Integer, minValue: 0, maxValue: 127),
                    new ColumnDefinition(nameof(Condition), ColumnType.String, 0, false, true, ColumnCategory.Condition, modularizeType: ColumnModularizeType.Condition),
                    new ColumnDefinition(nameof(Order), ColumnType.Number, 4, false, false, ColumnCategory.Integer, minValue: 0, maxValue: int.MaxValue),
                };
            }
        }

        public PSW_DeletePath() : base(SymbolDefinition)
        { }

        public PSW_DeletePath(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "dlt")
        { }

        public string Path
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public int Flags
        {
            get => Fields[1].AsNumber();
            set => this.Set(1, value);
        }

        public string Condition
        {
            get => Fields[2].AsString();
            set => this.Set(2, value);
        }

        public int Order
        {
            get => Fields[3].AsNumber();
            set => this.Set(3, value);
        }
    }
}
