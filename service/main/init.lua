print("run lua init.lua")

function OnInit(id)
    server.NewService("chat")
end

function OnExit()
    print("[lua] main OnExit")
end