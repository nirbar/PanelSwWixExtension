using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_ToLowerCase : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_ToLowerCase), CreateFieldDefinitions(ColumnDefinitions, 0), typeof(PSW_ToLowerCase));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Property_), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Column)
                };
            }
        }

        public PSW_ToLowerCase() : base(SymbolDefinition)
        { }

        public PSW_ToLowerCase(SourceLineNumber lineNumber, string property) : base(SymbolDefinition, lineNumber, new Identifier(AccessModifier.Global, property))
        {
            Property_ = property;
        }

        public string Property_
        {
            get => Fields[0].AsString();
            set => this.Set(0, value);
        }
    }
}
