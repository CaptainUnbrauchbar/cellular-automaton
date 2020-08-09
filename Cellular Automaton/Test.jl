using Random
using Dates

function t22()
    b::Float32 = 100
    println(b)
    println(convert(Int64, b))
end

rng = MersenneTwister(1234)
x = rand(rng)*100 >= 50
println(x)
println(typeof(x))

a = [1,2,3]
println(a[1])

t22()
println(sum(a[1:2]))

startTime1m = millisecond(now())
startTime1s = second(now())
const jobs = Channel{Tuple}(32)

const results = Channel{Tuple}(32)

function do_work()
          for job_id in jobs
              exec_time = rand()
              sleep(exec_time)                # simulates elapsed time doing actual work
                                              # typically performed externally.
              put!(results, (job_id, exec_time))
          end
          println("Ende")
      end;

#function make_jobs(n)
#          for i in 1:n
#              put!(jobs, i)
#          end
#      end;

  function make_jobs(x, y)
                for i in 1:x
                    for j in 1:y
                        put!(jobs, (i, j))
                    end
                end
            end;


n = 100;

@async make_jobs(10, 10); # feed the jobs channel with "n" jobs
println("Fertig")

for i in 1:4 # start 4 tasks to process requests in parallel
          @async do_work()
      end

@elapsed while n > 0 # print out results
          job_id, exec_time = take!(results)
          println("$job_id finished in $(round(exec_time; digits=2)) seconds")
          global n = n - 1
      end
#      startTime2m = millisecond(now())
#      startTime2s = second(now())
#
#
#      jobs2 = zeros(Int, 12)
#      n = 12
#      const results2 = Array{Tuple{Int64, Float64}}(undef, 12)
#
#      function do_work2()
#                for job_id in jobs2
#                    exec_time = rand()
#                    sleep(exec_time)                # simulates elapsed time doing actual work
#                                                    # typically performed externally.
#                    results2[job_id] = (job_id, exec_time)
#                end
#            end;
#
#      function make_jobs2(n, jobs2)
#                for i in 1:n
#                    jobs2[i] = i
#                end
#            end;
#
#
#
#      make_jobs2(n, jobs2); # feed the jobs channel with "n" jobs
#
#
#      for i in 1:4 # start 4 tasks to process requests in parallel
#                do_work2()
#            end
#
#      while n > 0 # print out results
#                job_id, exec_time = results2[n]
#                println("$job_id finished in $(round(exec_time; digits=2)) seconds")
#                global n = n - 1
#            end
#
#            startTime3m = millisecond(now())
#            startTime3s = second(now())
#
#println(startTime2m - startTime1m, " ", startTime2s - startTime1s)
#println(startTime3m - startTime2m, " ", startTime3s - startTime2s)

using Images, TestImages, LinearAlgebra, Interact

#img = testimage("mandrill")
#channels = float(channelview(img))
#function rank_approx(F::SVD, k)
#    U, S, V = F
#    M = U[:, 1:k] * Diagonal(S[1:k]) * V[:, 1:k]'
#    M = min.(max.(M, 0.0), 1.)
#end
#svdfactors = (svd(channels[1,:,:]), svd(channels[2,:,:]), svd(channels[3,:,:]))
#
#n = 100
#@manipulate for k1 in 1:n, k2 in 1:n, k3 in 1:n
#    colorview(RGB,
#              rank_approx(svdfactors[1], k1),
#              rank_approx(svdfactors[2], k2),
#              rank_approx(svdfactors[3], k3)
#              )
#end

img = rand(5,5)
Gray.(img)
