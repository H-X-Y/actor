local serviceId
local conns = {}

function OnInit(id)
    serviceId = id
    print("[lua] chat OnInit id:"..id)
    server.Listen(8005, id)
end


function OnAcceptMsg(listenfd, clientfd)
    print("[lua] chat OnAcceptMsg "..clientfd)
    conns[clientfd] = true
end


function OnSocketData(fd, buff)
    print("[lua] chat OnSocketData "..fd)
    for fd, _ in pairs(conns) do
        server.Write(fd, buff)
    end
end

function OnSocketClose(fd)
    print("[lua] chat OnSocketClose "..fd)
    conns[fd] = nil
end