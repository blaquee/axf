PluginInterface @pi;

void OnFinalize(ptr p)
{
    pi.Log("ITS GONE");
}

int main(PluginInterface @p)
{
    @pi = p;
    
    //ptr k32 = pi.GetModuleBase("kernel32.dll");
    //pi.Log(pi.GetProcAddress(k32, "ReadProcessMemory"));
    
    string[] plist = pi.GetEventList();
    for(uint i = 0;i < plist.length(); ++i)
        pi.Log(plist[i]);

    pi.Log(pi.SubscribeEvent("OnFinalize", @OnFinalize));
    
    return 0x10000; // version 1.0
}

string IntToStr(int64 i)
{
    return formatInt(i, "");
}
string IntToHex(int64 i)
{
    return formatInt(i, "H");
}
