using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_SetPropertyFromPipe : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_SetPropertyFromPipe), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_SetPropertyFromPipe));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column)
                    , new ColumnDefinition(nameof(PipeName), ColumnType.Localized, 0, false, false, ColumnCategory.Formatted, modularizeType: ColumnModularizeType.Property)
                    , new ColumnDefinition(nameof(Timeout), ColumnType.Number, 4, false, false, ColumnCategory.Integer, 0, int.MaxValue)
                };
            }
        }

        public PSW_SetPropertyFromPipe() : base(SymbolDefinition)
        { }

        public PSW_SetPropertyFromPipe(SourceLineNumber lineNumber) : base(SymbolDefinition, lineNumber, "spp")
        { }

        public string PipeName
        {
            get => Fields[0].AsString();
            set => Fields[0].Set(value);
        }

        public int Timeout
        {
            get => Fields[1].AsNumber();
            set => Fields[1].Set(value);
        }
    }
}
