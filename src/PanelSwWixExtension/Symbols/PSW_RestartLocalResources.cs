using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_RestartLocalResources : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_RestartLocalResources), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_RestartLocalResources));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                    new ColumnDefinition(nameof(Path), ColumnType.Localized, 72, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property),
                    new ColumnDefinition(nameof(Condition), ColumnType.String, 0, false, true, ColumnCategory.Condition, modularizeType: ColumnModularizeType.Condition),
                };
            }
        }

        public PSW_RestartLocalResources() : base(SymbolDefinition)
        { }

        public PSW_RestartLocalResources(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "rml")
        { }

        public string Path
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public string Condition
        {
            get => Fields[1].AsString();
            set => this.Set(1, value);
        }
    }
}
