using System;
using WixToolset.Data;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_ContainerExtensionData : BaseSymbol
    {
        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_ContainerExtensionData),
                    new IntermediateFieldDefinition[]
                    {
                        new IntermediateFieldDefinition(nameof(ContainerId), IntermediateFieldType.String),
                        new IntermediateFieldDefinition(nameof(Compression), IntermediateFieldType.String),
                    }
                    , typeof(PSW_ContainerExtensionData));
            }
        }

        public PSW_ContainerExtensionData(SourceLineNumber sourceLineNumber)
            : base(SymbolDefinition, sourceLineNumber, new Identifier(AccessModifier.Global, 0))
        {
        }

        public string ContainerId
        {
            get => (string)this.Fields[0].AsString();
            set => this.Set(0, value);
        }

        public PSW_ContainerTemplate.ContainerCompressionType Compression
        {
            get
            {
                string v = this.Fields[1].AsString();
                return !string.IsNullOrEmpty(v) && Enum.TryParse(v, out PSW_ContainerTemplate.ContainerCompressionType type) ? type : PSW_ContainerTemplate.ContainerCompressionType.Cab;
            }
            set => this.Set(1, value.ToString());
        }
    }
}
