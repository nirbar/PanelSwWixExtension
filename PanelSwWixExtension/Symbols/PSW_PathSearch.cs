using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_PathSearch : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_PathSearch), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_PathSearch));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(Property_), ColumnType.String, 0, false, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(FileName), ColumnType.String, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                };
            }
        }

        public PSW_PathSearch() : base(SymbolDefinition)
        { }

        public PSW_PathSearch(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "pth")
        { }

        public string Property_
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public string FileName
        {
            get => Fields[1].AsString();
            set => this.Set(1, value);
        }
    }
}
