using System;
using WixToolset.Data;
using WixToolset.Data.Burn;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_BundleVariableSearch : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                IntermediateSymbolDefinition symbol = new IntermediateSymbolDefinition(nameof(PSW_BundleVariableSearch),
                    new IntermediateFieldDefinition[]
                    {
                        new IntermediateFieldDefinition(nameof(SearchVariable), IntermediateFieldType.String),
                        new IntermediateFieldDefinition(nameof(UpgradeCode), IntermediateFieldType.String),
                        new IntermediateFieldDefinition(nameof(Format), IntermediateFieldType.Number),
                    }
                    , typeof(PSW_BundleVariableSearch));
                symbol.AddTag(BurnConstants.BootstrapperExtensionSearchSymbolDefinitionTag);
                return symbol;
            }
        }

        public PSW_BundleVariableSearch(SourceLineNumber sourceLineNumber, Identifier id)
            : base(SymbolDefinition, sourceLineNumber, id)
        {
        }

        public string SearchVariable
        {
            get => this.Fields[0].AsString();
            set => this.Set(0, value);
        }

        public string UpgradeCode
        {
            get => this.Fields[1].AsString();
            set => this.Set(1, value);
        }

        public bool Format
        {
            get => this.Fields[2].AsNumber() != 0;
            set => this.Set(2, value ? 1 : 0);
        }
    }
}
