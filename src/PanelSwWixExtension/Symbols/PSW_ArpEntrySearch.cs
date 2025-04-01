using WixToolset.Data;
using WixToolset.Data.Burn;
using static PanelSw.Wix.Extensions.PanelSwWixCompiler;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_ArpEntrySearch : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                IntermediateSymbolDefinition symbol = new IntermediateSymbolDefinition(nameof(PSW_ArpEntrySearch),
                    new IntermediateFieldDefinition[]
                    {
                        new IntermediateFieldDefinition(nameof(ArpSpec), IntermediateFieldType.String),
                        new IntermediateFieldDefinition(nameof(Bitness), IntermediateFieldType.Number),
                        new IntermediateFieldDefinition(nameof(IsUserContext), IntermediateFieldType.Number),
                    }
                    , typeof(PSW_ArpEntrySearch));
                symbol.AddTag(BurnConstants.BootstrapperExtensionSearchSymbolDefinitionTag);
                return symbol;
            }
        }

        public PSW_ArpEntrySearch(SourceLineNumber sourceLineNumber, Identifier id)
            : base(SymbolDefinition, sourceLineNumber, id)
        {
        }

        public string ArpSpec
        {
            get => this.Fields[0].AsString();
            set => this.Set(0, value);
        }

        public Bitness Bitness
        {
            get => (Bitness)this.Fields[1].AsNumber();
            set => this.Set(1, (int)value);
        }

        public bool IsUserContext
        {
            get => this.Fields[2].AsNumber() != 0;
            set => this.Set(2, value ? 1 : 0);
        }
    }
}
