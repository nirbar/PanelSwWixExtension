using System.Collections.Generic;
using WixToolset.Data;
using WixToolset.Data.WindowsInstaller;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_PropertyPersist : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_PropertyPersist), CreateFieldDefinitions(ColumnDefinitions), typeof(PSW_PropertyPersist));
            }
        }
        public static IEnumerable<ColumnDefinition> ColumnDefinitions
        {
            get
            {
                return new ColumnDefinition[]
                {
                    new ColumnDefinition(nameof(Id), ColumnType.String, 72, true, false, ColumnCategory.Identifier, modularizeType: ColumnModularizeType.Property),
                };
            }
        }

        public PSW_PropertyPersist() : base(SymbolDefinition)
        { }

        public PSW_PropertyPersist(SourceLineNumber lineNumber, string propertyName) : base(SymbolDefinition, lineNumber, new Identifier(AccessModifier.Global, propertyName))
        { }
    }
}
