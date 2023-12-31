using WixToolset.Data;

namespace PanelSw.Wix.Extensions
{
    public static class PanelSwWixErrorMessages
    {
        public static Message ExecuteCommandSequence(SourceLineNumber sourceLineNumber, string executeCommandId, string otherActionId)
        {
            return new Message(sourceLineNumber, MessageLevel.Error, (int)PswErrorId.ExecuteCommandSequence, MessageResources.ExecuteCommandSequence, executeCommandId, otherActionId);
        }
        public static Message PayloadExceedsSize(SourceLineNumber sourceLineNumber, string payloadId, long maxSize)
        {
            return new Message(sourceLineNumber, MessageLevel.Warning, (int)PswErrorId.PayloadExceedsSize, MessageResources.PayloadExceedsSize, payloadId, maxSize);
        }
    }

    public enum PswErrorId : int
    {
        ExecuteCommandSequence = 9000,
        PayloadExceedsSize,
    }
}
