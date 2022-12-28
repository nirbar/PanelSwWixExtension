using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_Payload : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_Payload), CreateFieldDefinitions(ColumnDefinitions, 0), typeof(PSW_Payload));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(BinaryKey_), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column, keyTable: "Binary", keyColumn: 1),
                    new ColumnDefinition(nameof(Name), ColumnType.String, 0, false, false, ColumnCategory.AnyPath),
                };
            }
        }

        public PSW_Payload() : base(SymbolDefinition)
        { }

        public PSW_Payload(SourceLineNumber lineNumber, string binaryKey) : base(SymbolDefinition, lineNumber, new Identifier(AccessModifier.Global, binaryKey))
        { }

        public string BinaryKey_
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }

        public string Name
        {
            get => Fields[1].AsString();
            set => this.Set(1, value);
        }
    }
}
