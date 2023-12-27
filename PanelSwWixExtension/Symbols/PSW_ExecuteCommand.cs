using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_ExecuteCommand : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_ExecuteCommand), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_ExecuteCommand));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column),
                };
            }
        }

        public PSW_ExecuteCommand() : base(SymbolDefinition)
        { }

        public PSW_ExecuteCommand(SourceLineNumber lineNumber, Identifier id) : base(SymbolDefinition, lineNumber, id)
        { }
    }
}
