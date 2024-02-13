internal class ContainsRule : RegExRule
{

    public ContainsRule(string text)
        : base($"\\b\\w*{text}\\w*\\b")
    {
    }

}
