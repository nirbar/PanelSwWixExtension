using WixToolset.Data;
using WixToolset.Data.Burn;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_CustomSearch : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                IntermediateSymbolDefinition symbol = new IntermediateSymbolDefinition(nameof(PSW_CustomSearch),
                    new IntermediateFieldDefinition[]
                    {
                        new IntermediateFieldDefinition(nameof(BundleExtensionRef), IntermediateFieldType.String),
                        new IntermediateFieldDefinition(nameof(BundleExtensionData), IntermediateFieldType.String),
                    }
                    , typeof(PSW_CustomSearch));
                symbol.AddTag(BurnConstants.BundleExtensionSearchSymbolDefinitionTag);
                return symbol;
            }
        }

        public PSW_CustomSearch(SourceLineNumber sourceLineNumber, Identifier id)
            : base(SymbolDefinition, sourceLineNumber, id)
        {
        }

        public string BundleExtensionRef
        {
            get => this.Fields[0].AsString();
            set => this.Set(0, value);
        }

        public string BundleExtensionData
        {
            get => this.Fields[1].AsString();
            set => this.Set(1, value);
        }
    }
}
