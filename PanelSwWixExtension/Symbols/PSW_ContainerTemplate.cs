using System;
using WixToolset.Data;
using WixToolset.Data.Symbols;

namespace PanelSw.Wix.Extensions.Symbols
{
    internal class PSW_ContainerTemplate : BaseSymbol
    {
        public enum ContainerCompressionType
        {
            Zip,
            Cab,
        }

        public static IntermediateSymbolDefinition SymbolDefinition
        {
            get
            {
                return new IntermediateSymbolDefinition(nameof(PSW_ContainerTemplate),
                    new IntermediateFieldDefinition[]
                    {
                        new IntermediateFieldDefinition(nameof(CabinetTemplate), IntermediateFieldType.String),
                        new IntermediateFieldDefinition(nameof(DefaultType), IntermediateFieldType.Number),
                        new IntermediateFieldDefinition(nameof(MaximumUncompressedContainerSize), IntermediateFieldType.LargeNumber),
                        new IntermediateFieldDefinition(nameof(MaximumUncompressedExeSize), IntermediateFieldType.LargeNumber),
                        new IntermediateFieldDefinition(nameof(Compression), IntermediateFieldType.Number),
                    }
                    , typeof(PSW_ContainerTemplate));
            }
        }

        public PSW_ContainerTemplate(SourceLineNumber sourceLineNumber)
            : base(SymbolDefinition, sourceLineNumber, new Identifier(AccessModifier.Global, 0))
        {
        }

        public string CabinetTemplate
        {
            get => (string)this.Fields[0].AsString();
            set => this.Set(0, value);
        }

        public ContainerType DefaultType
        {
            get => (ContainerType)this.Fields[1].AsNumber();
            set => this.Set(1, (int)value);
        }

        public long MaximumUncompressedContainerSize
        {
            get => this.Fields[2].AsLargeNumber();
            set => this.Set(2, value);
        }

        public long MaximumUncompressedExeSize
        {
            get => this.Fields[3].AsLargeNumber();
            set => this.Set(3, value);
        }

        public ContainerCompressionType Compression
        {
            get
            {
                int v = this.Fields[4].AsNumber();
                return (ContainerCompressionType)v;
            }
            set => this.Set(4, (int)value);
        }
    }
}
