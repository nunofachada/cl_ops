%   
% This file is part of CL-Ops.
% 
% CL-Ops is free software: you can redistribute it and/or modify
% it under the terms of the GNU General Public License as published by
% the Free Software Foundation, either version 3 of the License, or
% (at your option) any later version.
% 
% CL-Ops is distributed in the hope that it will be useful, 
% but WITHOUT ANY WARRANTY; without even the implied warranty of
% MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
% GNU General Public License for more details.
% 
% You should have received a copy of the GNU General Public License
% along with CL-Ops.  If not, see <http://www.gnu.org/licenses/>.
%
 
% Octave/Matlab script for ploting random number generator outputs

% Load random values generated with MT host-based seeds
load('out_lcg_host.tsv');
load('out_xorshift64_host.tsv');
load('out_xorshift128_host.tsv');
load('out_mwc64x_host.tsv');

% New figure 
figure;
% Plot random values generated with MT host-based seeds
subplot(2,2,1);imagesc(out_lcg_host);title('lcg');
subplot(2,2,2);imagesc(out_mwc64x_host);title('mwc64x');
subplot(2,2,3);imagesc(out_xorshift64_host);title('xorshift64');
subplot(2,2,4);imagesc(out_xorshift128_host);title('xorshift128');
 
% Optionally use another colormap
% colormap(bone)

% Load random values generated with workitem global IDs
load('out_mwc64x_gid.tsv');
load('out_lcg_gid.tsv');
load('out_xorshift64_gid.tsv');
load('out_xorshift128_gid.tsv');

% New figure
figure
% Plot random values generated with workitem global IDs
subplot(2,2,1);imagesc(out_lcg_gid);title('lcg');
subplot(2,2,2);imagesc(out_mwc64x_gid);title('mwc64x');
subplot(2,2,3);imagesc(out_xorshift64_gid);title('xorshift64');
subplot(2,2,4);imagesc(out_xorshift128_gid);title('xorshift128');



% Wait before quiting (useful for running this as an external octave
% script)
pause;
