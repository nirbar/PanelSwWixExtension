using WixToolset.Data;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_Container : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_Container),
                    new IntermediateFieldDefinition[]
                    {
                        new IntermediateFieldDefinition(nameof(CompressionLevel), IntermediateFieldType.Number),
                    }
                    , typeof(PSW_Container));
            }
        }

        public PSW_Container(SourceLineNumber sourceLineNumber, Identifier id)
            : base(SymbolDefinition, sourceLineNumber, id)
        {
        }

        public CompressionLevel CompressionLevel
        {
            get
            {
                int v = this.Fields[0].AsNumber();
                return (CompressionLevel)v;
            }
            set => this.Set(0, (int)value);
        }
    }
}
