using System;
using System.Collections.Generic;
using System.Text;
using System.Xml.Serialization;

namespace PswManagedCA
{
    [Serializable]
    public class ZipFileCatalog
    {
        public ZipFileCatalog()
        {
            Sources = new List<ZipSource>();
        }

        [Serializable]
        public class ZipSource
        {
            public string Id { get; set; }
            public string SrcFolder { get; set; }
            public string Pattern { get; set; }
            public bool Recursive { get; set; }
        }

        public List<ZipSource> Sources { get; set; }

        public string DstZipFile { get; set; }
    }
}
