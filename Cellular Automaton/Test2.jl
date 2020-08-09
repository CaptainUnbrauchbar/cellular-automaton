using Dates # für die Zeit
using Distributed # für die Paraalelität
using Images # zum Zeichnen

const jobs = RemoteChannel(()->Channel{Int}(32));

const results = RemoteChannel(()->Channel{Tuple}(32));

addprocs(4)

@everywhere function do_work(jobs, results) # define work function everywhere
   println("Hier")
   while true
       job_id = take!(jobs)
       exec_time = rand()
       sleep(exec_time) # simulates elapsed time doing actual work
       put!(results, (job_id, exec_time, myid()))
   end
end

function make_jobs(n)
   for i in 1:n
       put!(jobs, i)
   end
end

println("Test")

println("Test2")

n = 12
println("Test3")
@async make_jobs(n); # feed the jobs channel with "n" jobs
println(workers())
for p in workers() # start tasks on the workers to process requests in parallel
   remote_do(do_work, p, jobs, results)
end
println("Test5")
for index in 1:n # print out results
   job_id, exec_time, where = take!(results)
   println("$job_id finished in $(round(exec_time; digits=2)) seconds on worker $where")
end
#const jobs = RemoteChannel(()->Channel{Int}(32));

#const results = RemoteChannel(()->Channel{Tuple}(32));


#@everywhere function do_work(jobs, results) # define work function everywhere
#    println("Test")
#   while true
#       job_id = take!(jobs)
#       exec_time = rand()
#       sleep(exec_time) # simulates elapsed time doing actual work
#       put!(results, (job_id, exec_time, myid()))
#   end
#end
##
#function make_jobs(n)
 #  for i in 1:n
#       put!(jobs, i)
#   end
#end;
#
#
#addprocs(4); # add worker processes

#n = 12;
#
#@async make_jobs(n); # feed the jobs channel with "n" jobs
#
#for p in workers() # start tasks on the workers to process requests in parallel
#    remote_do(do_work, p, jobs, results)
#end
#
#@elapsed while n > 0 # print out results
#    job_id, exec_time, where = take!(results)
#    println("$job_id finished in $(round(exec_time; digits=2)) seconds on worker $where")
#    global n = n - 1
#end
#
