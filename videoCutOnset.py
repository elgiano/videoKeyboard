import sys,os,os.path

file = sys.argv[1]
outdir = sys.argv[2]
name = os.path.basename(file)
extension = os.path.splitext(name)[1]

os.system("ffmpeg -i "+sys.argv[1]+" -map 0:a "+name+".audio.wav -map 0:v "+name+".onlyvideo.avi")
os.system("rm "+name+".onlyvideo.avi")
os.system("aubiocut "+name+".audio.wav > "+name+".onsets")
os.system("rm "+name+".audio.wav")

f = open(name+".onsets","r"); o = [float(line) for line in f.readlines()];

if not os.path.isdir(outdir):
    os.mkdir(outdir)

i=0
onsetDurs = [list(zip(o[:-1],[o[i+1]-s for i,s in enumerate(o[:-1])]))][0]
for start,dur in onsetDurs:
    os.system("ffmpeg -ss "+str(start)+" -i "+file+" -t "+str(dur)+" -vcodec copy -acodec copy "+os.path.join(outdir,i.zfill(3))+extension)
    i=i+1
os.system("rm "+name+".onsets")
